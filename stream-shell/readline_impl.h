#include "readline.h"

#include <readline/history.h>
#include <readline/readline.h>

#include <sys/select.h>
#include <unistd.h>

#include <atomic>
#include <clocale>
#include <cstring>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>

class ReadlinePromptImpl : public ReadlinePrompt {
 public:
  ReadlinePromptImpl() {
    setlocale(LC_ALL, "");

    if (pipe(_pipefd) != 0) {
      throw std::runtime_error("pipe failed");
    }

    _running = true;
    _thread = std::thread(&ReadlinePromptImpl::threadMain, this);
  }

  ~ReadlinePromptImpl() {
    _running = false;
    wake();

    if (_thread.joinable()) {
      _thread.join();
    }

    close(_pipefd[0]);
    close(_pipefd[1]);
  }

  std::future<std::optional<std::string>> prompt(std::string_view prompt) override {
    auto promise = std::promise<std::optional<std::string>>();
    auto fut = promise.get_future();

    {
      std::lock_guard lock(_mtx);
      _cmds.push(Cmd{CmdType::kStartPrompt, std::string(prompt), std::move(promise)});
    }

    wake();
    return fut;
  }

  void refresh(std::string_view prompt) override {
    {
      std::lock_guard lock(_mtx);
      _cmds.push(Cmd{CmdType::kRefreshPrompt, std::string(prompt), {}});
    }

    wake();
  }

  void interrupt() override {
    {
      std::lock_guard lock(_mtx);
      _cmds.push(Cmd{CmdType::kInterrupt, {}, {}});
    }

    wake();
  }

 private:
  enum class CmdType { kStartPrompt, kRefreshPrompt, kInterrupt };

  struct Cmd {
    CmdType type;
    std::string data;
    std::optional<std::promise<std::optional<std::string>>> promise;
  };

  static ReadlinePromptImpl *_self;  // for static callback

  static void onLine(char *line) {
    if (!_self || !_self->_current_promise) {
      return;
    }
    auto promise = *std::exchange(_self->_current_promise, {});

    if (line) {
      promise.set_value(line);
      add_history(line);
      free(line);

    } else {
      promise.set_value({});
    }
    rl_callback_handler_remove();
  }

  void threadMain() {
    _self = this;

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

      if (FD_ISSET(STDIN_FILENO, &fds) && _current_promise) {
        rl_callback_read_char();
      }

      if (FD_ISSET(_pipefd[0], &fds)) {
        char buf[32];
        read(_pipefd[0], buf, sizeof(buf));
        handle_cmds();
      }
    }

    rl_callback_handler_remove();
    _self = nullptr;
  }

  void handle_cmds() {
    std::queue<Cmd> local;

    {
      std::lock_guard lock(_mtx);
      std::swap(local, _cmds);
    }

    while (!local.empty()) {
      auto cmd = std::move(local.front());
      local.pop();

      switch (cmd.type) {
        case CmdType::kStartPrompt:
          _prompt = std::move(cmd.data);

          if (cmd.promise) {
            if (auto prev_promise = std::exchange(_current_promise, std::move(*cmd.promise))) {
              prev_promise->set_value({});
              rl_newline(1, '\n');

            } else {
              rl_callback_handler_install(_prompt.c_str(), &ReadlinePromptImpl::onLine);
              break;
            }
          }

          rl_set_prompt(_prompt.c_str());
          rl_redisplay();
          break;

        case CmdType::kRefreshPrompt:
          _prompt = std::move(cmd.data);

          rl_set_prompt(_prompt.c_str());
          rl_redisplay();
          break;

        case CmdType::kInterrupt:
          if (auto prev_promise = std::exchange(_current_promise, {})) {
            prev_promise->set_value({});
            rl_newline(1, '\n');
            rl_callback_handler_remove();
          }
          break;
      }
    }
  }

  void wake() { write(_pipefd[1], "x", 1); }

  std::thread _thread;
  std::atomic<bool> _running = false;

  int _pipefd[2];

  std::mutex _mtx;
  std::queue<Cmd> _cmds;

  std::optional<std::promise<std::optional<std::string>>> _current_promise;

  std::string _prompt;
};

// static
ReadlinePromptImpl *ReadlinePromptImpl::_self = nullptr;
