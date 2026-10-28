#include "stream_printer.h"

#include <fstream>
#include <iostream>
#include <string_view>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <range/v3/all.hpp>
#include <unistd.h>
#include "readline.h"
#include "to_string.h"

namespace {

struct Consumer {
  virtual ~Consumer() = default;
  virtual auto operator()(size_t i, const Value &) -> bool = 0;
};

struct Printer : Consumer {
  virtual ~Printer() = default;
  virtual bool print(size_t i, std::string_view value) = 0;

  auto operator()(size_t i, const Value &value) -> bool override {
    auto str = std::visit(_to_str, value);
    return str && print(i, *str);
  }

  ToString::Value _to_str;
};

#if 0

struct SlicePrinter final : Printer {
  explicit SlicePrinter(size_t window) : _window{window} {}

  bool print(size_t i, std::string_view value) override {
    rollback();
    pushValue(value);

    for (_lines_printed = 0; auto &value : _scrollback) {
      std::cout << value << std::endl;
      _lines_printed += ranges::count(value, '\n') + 1;
    }
    return true;
  }

 private:
  void rollback() {
    ranges::for_each(ranges::views::iota(0, _lines_printed),
                     [](auto) { std::cout << "\033[A\033[K"; });
  }
  void pushValue(std::string_view value) {
    _scrollback.emplace_back(value);
    if (_scrollback.size() > _window) _scrollback.pop_front();
  }

  size_t _window;
  std::deque<std::string> _scrollback;
  int _lines_printed = 0;
};

#endif

struct REPLPrinter final : Printer {
  REPLPrinter(ReadlinePrompt &prompt) : _prompt{prompt} {}

  bool print(size_t i, std::string_view value) override {
    setBuffer(value);

    for (;;) {
      printBuffer();

      if (auto result = _prompt.prompt({}, {'\t'})) {
        if (result->trigger == '\t') {
          toggleMode();
          rollback();
          continue;
        }
        return true;
      } else {
        std::cout << std::endl;
        return false;
      }
    }
  }

  void setBuffer(std::string_view value) {
    _buffer = value;
    _lines_printed = ranges::count(value, '\n');
  }

  void rollback() {
    std::cout << "\033[G\033[K";
    ranges::for_each(ranges::views::iota(0, _lines_printed),
                     [](auto) { std::cout << "\033[A\033[K"; });
  }

  void printBuffer() {
    switch (0) {
      case 0:
        std::cout << _buffer << std::flush;
        break;
      default:
        assert(0);
    }
  }

  void toggleMode() {
    // todo
  }

  ReadlinePrompt &_prompt;
  std::string _buffer;
  int _lines_printed = 0;
};

}  // namespace

void printStream(Stream &&stream, ReadlinePrompt &prompt) {
  auto consumer = std::make_unique<REPLPrinter>(prompt);
  if (!consumer) {
    return;
  }
  for (auto &&[i, result] : ranges::views::enumerate(std::move(stream))) {
    if (!result) {
      if (result.error() != Error::kAborted) {
        std::cerr << std::format("Failed with code: {}", int(result.error())) << std::endl;
      }
      return;
    } else if (!(*consumer)(i, *result)) {
      return;
    }
  }
}
