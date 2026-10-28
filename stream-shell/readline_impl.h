#include "readline.h"

#include <readline/history.h>
#include <readline/readline.h>

#include <sys/select.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <clocale>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>

class ReadlinePromptImpl : public ReadlinePrompt {
 public:
  ReadlinePromptImpl() {
    setlocale(LC_ALL, "");

    if (pipe(_pipefd) != 0) {
      throw std::runtime_error("pipe failed");
    }
  }

  ~ReadlinePromptImpl() {
    close(_pipefd[0]);
    close(_pipefd[1]);
  }

  static ReadlinePromptImpl *_self;  // for static callback

  static void onLine(char *line) {
    if (!_self) {
      return;
    }

    if (line) {
      _self->_line = line;
      add_history(line);
      free(line);

    } else {
      _self->_line.reset();
    }
    _self->_running = false;
    rl_callback_handler_remove();
  }

  static int onTrigger(int count, int key) {
    if (!_self) {
      return -1;
    }
    _self->_trigger = key;
    rl_done = 1;
    return 0;
  }

  std::optional<Result> prompt(Prompt prompt, Triggers triggers) override {
    _running = true;
    _self = this;

    std::ranges::for_each(
        triggers, [](auto trigger) { rl_bind_key(trigger, &ReadlinePromptImpl::onTrigger); });

    // todo: subscribe and refresh on background thread
    // auto refresh = [](auto prompt) {
    //   rl_set_prompt(prompt.c_str());
    //   rl_redisplay();
    // };

    auto prompt0 = [&] -> std::string {
      auto strs = prompt | ranges::views::take(1) | ranges::to<std::vector>;
      if (strs.empty()) {
        return {};
      }
      return strs.front();
    }();

    rl_callback_handler_install(prompt0.c_str(), &ReadlinePromptImpl::onLine);

    while (_running) {
      fd_set fds;
      FD_ZERO(&fds);

      FD_SET(STDIN_FILENO, &fds);
      FD_SET(_pipefd[0], &fds);

      int maxfd = std::max(STDIN_FILENO, _pipefd[0]);

      int r = select(maxfd + 1, &fds, nullptr, nullptr, nullptr);
      if (r < 0) {
        if (errno == EINTR) continue;
        break;
      }

      if (FD_ISSET(STDIN_FILENO, &fds) && _running) {
        rl_callback_read_char();
      }

      if (FD_ISSET(_pipefd[0], &fds)) {
        char buf[32];
        read(_pipefd[0], buf, sizeof(buf));
      }
    }

    std::ranges::for_each(triggers, rl_unbind_key);

    rl_callback_handler_remove();
    _self = nullptr;

    return std::exchange(_line, {}).transform([&](auto line) {
      return Result{.value = std::move(line), .trigger = std::exchange(_trigger, {})};
    });
  }

  void interrupt() override {
    _running = false;
    wake();
  }

  void wake() { write(_pipefd[1], "x", 1); }

  std::atomic_bool _running;
  int _pipefd[2];

  std::optional<std::string> _line;
  int _trigger = {};
};

// static
ReadlinePromptImpl *ReadlinePromptImpl::_self = nullptr;
