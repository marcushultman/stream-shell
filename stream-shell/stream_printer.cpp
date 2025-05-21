#include "stream_printer.h"

#include <fstream>
#include <iostream>
#include <string_view>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <range/v3/all.hpp>
#include <unistd.h>
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

struct REPLPrinter final : Printer {
  REPLPrinter(bool all, const Prompt &prompt) : _all{all}, _prompt{prompt} {}

  bool print(size_t i, std::string_view value) override {
    if (i == 0 || _all) {
      std::cout << value << std::endl;
      return true;
    } else if (auto line = _prompt("Next [Enter]")) {
      std::cout << value << std::endl;
      _all = line == std::string_view(":");
      return true;
    }
    return false;
  }
  bool _all = false;
  const Prompt &_prompt;
};

struct FileWriter final : Consumer {
  FileWriter(std::ofstream file) : _file{std::move(file)} {}

  auto operator()(size_t, const Value &value) -> bool override {
    auto str = *std::visit(_to_str, value);
    _file.write(str.data(), str.size());
    return true;
  }

  std::ofstream _file;
  ToString::Value _to_str;
};

struct ConsumerFactory {
  using C = std::unique_ptr<Consumer>;
  ConsumerFactory(const Prompt &prompt) : _prompt{prompt} {}
  auto operator()(Print::Pull &pull) -> C {
    return std::make_unique<REPLPrinter>(pull.full, _prompt);
  }
  auto operator()(Print::Slice &slice) -> C { return std::make_unique<SlicePrinter>(slice.window); }
  auto operator()(Print::WriteFile &write_file) -> C {
    auto filename = write_file.filename | ranges::to<std::string>;
    auto file = std::ofstream(filename);
    if (!file.is_open()) {
      std::cerr << std::format("Failed to open file: {}", filename) << std::endl;
      return nullptr;
    }
    return std::make_unique<FileWriter>(std::move(file));
  }
  const Prompt &_prompt;
};

void printStream(Stream &&stream, Print::Mode mode, const Prompt &prompt) {
  auto consumer = std::visit(ConsumerFactory(prompt), mode);
  if (!consumer) {
    return;
  }
  for (auto &&[i, result] : ranges::views::enumerate(std::move(stream))) {
    if (!result) {
      std::cerr << std::format("Failed with code: {}", int(result.error())) << std::endl;
      return;
    } else if (!(*consumer)(i, *result)) {
      return;
    }
  }
}

}  // namespace

void printStream(PrintableStream &&stream, const Prompt &prompt) {
  printStream(std::move(stream.first), stream.second, prompt);
}
