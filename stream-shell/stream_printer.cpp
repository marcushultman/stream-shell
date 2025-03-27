#include "stream_printer.h"

#include <iostream>
#include <string_view>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>
#include <range/v3/all.hpp>
#include <unistd.h>
#include "linenoise.h"

namespace {

struct Printer {
  virtual ~Printer() = default;
  virtual bool print(size_t i, std::string_view value) = 0;
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
  explicit REPLPrinter(bool all) : _all{all} {}

  bool print(size_t i, std::string_view value) override {
    if (i == 0 || _all) {
      std::cout << value << std::endl;
      return true;
    } else if (auto *line = linenoise("Next [Enter]")) {
      std::cout << value << std::endl;
      _all = line == std::string_view(":");
      return true;
    }
    return false;
  }
  bool _all = false;
};

struct ToJSON {
  auto operator()(const auto &value) {
    scratch.clear();
    if constexpr (std::is_same_v<std::decay_t<decltype(value)>, google::protobuf::Value>) {
      if (value.has_string_value()) {
        scratch = value.string_value();
        return absl::Status();
      }
    }
    return google::protobuf::util::MessageToJsonString(value, &scratch);
  }
  std::string scratch;
};

auto printStream(Stream &&stream, Print::Mode mode) -> std::expected<void, std::string> {
  ToJSON to_json;
  std::unique_ptr<Printer> printer;

  if (auto slice = std::get_if<Print::Slice>(&mode)) {
    printer = std::make_unique<SlicePrinter>(slice->window);
  } else {
    printer = std::make_unique<REPLPrinter>(std::get<Print::Pull>(mode).full);
  }

  for (auto &&[i, result] : ranges::views::enumerate(std::move(stream))) {
    if (!result) {
      return std::unexpected(std::format("Failed with code: {}", int(result.error())));
    }
    if (auto status = std::visit(to_json, *result); !status.ok()) {
      return std::unexpected(std::string(status.message()));
    } else if (!printer->print(i, to_json.scratch)) {
      break;
    }
  }
  return {};
}

}  // namespace

void printStream(PrintableStream &&stream) {
  if (auto status = printStream(std::move(stream.first), stream.second); !status.has_value()) {
    std::cerr << status.error() << std::endl;
  }
}
