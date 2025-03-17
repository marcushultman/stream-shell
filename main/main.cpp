#include <iostream>
#include <ranges>
#include <string_view>
#include <variant>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <range/v3/algorithm/contains.hpp>
#include <range/v3/all.hpp>
#include "linenoise.h"

// util/trim.h

std::string_view trimLeft(std::string_view str) {
  size_t start = str.find_first_not_of(" \t\n\r\f\v");
  return (start == std::string_view::npos) ? "" : str.substr(start);
}

std::string_view trimRight(std::string_view str) {
  size_t end = str.find_last_not_of(" \t\n\r\f\v");
  return (end == std::string_view::npos) ? "" : str.substr(0, end + 1);
}

std::string_view trim(std::string_view str) {
  return trimLeft(trimRight(str));
}

// util/to_string_view.h

auto toStringView(ranges::range auto &&s) -> std::string_view {
  return std::string_view(&*s.begin(), ranges::distance(s));
}

// value.h

struct Value {
  using InnerValue = std::variant<google::protobuf::Any, google::protobuf::StringValue>;

  Value(std::string_view str) {
    google::protobuf::StringValue value;
    value.set_value(str);
    _value = value;
  }

  friend auto &operator<<(std::ostream &os, const Value &);

  InnerValue _value;
};

auto &operator<<(std::ostream &os, const Value &v) {
  return os << std::get<google::protobuf::StringValue>(v._value).value();
}

// builtin.h

std::optional<ranges::any_view<Value>> findBuiltin(std::string_view cmd) {
  if (cmd.starts_with("iota")) {
    return ranges::views::iota(1) |
           ranges::views::transform([](auto i) { return Value("sheep " + std::to_string(i)); });
  }
  return {};
}

// command.h

struct Command {
  Command(auto str) : _str{trim(std::string_view(str))} {}

  auto run(ranges::range auto stream) -> ranges::any_view<Value> {
    if (_str.starts_with('{') && _str.ends_with('}')) {
      return ranges::views::single(Value("todo: closure"));
    } else if (auto builtin = findBuiltin(_str)) {
      return *builtin;
    }
    // todo: parse parenthesis
    return _str | ranges::views::split_when([&](char c) { return ranges::contains(" \n", c); }) |
           ranges::views::transform([](auto &&s) { return Value(toStringView(s)); });
  }

 private:
  std::string_view _str;
};

// pipeline.h

struct Pipeline {
  Pipeline(auto &&str, bool allow_stdout) : _allow_stdout{allow_stdout} {
    auto delim = std::min({str.find_last_of(':'), str.find_last_of('>'), str.size()});
    _commands_str = str.substr(0, delim);
    _consume_str = str.substr(_commands_str.size());
  }

  auto cmds() {
    return _commands_str | std::views::split('|') |
           std::views::transform([](auto &&cmd_str) { return Command(cmd_str); });
  }

  void consume(ranges::range auto &&stream) {
    auto ignore_output = [&] { ranges::for_each(stream, [](auto &&) {}); };

    if (_consume_str.starts_with(':')) {
      if (_allow_stdout) {
        if (_consume_str.substr(1) == "json") {
          std::cout << "[";
          for (auto &&[index, value] : ranges::views::enumerate(stream)) {
            // todo: format value
            std::cout << (index ? ", " : "") << "\"" << value << "\"";
          }
          std::cout << "]" << std::endl;
        } else {
          for (auto &&value : stream) {
            // todo: format value
            std::cout << value << std::endl;
          }
        }
      } else {
        ignore_output();
      }
    } else if (_consume_str.starts_with('>')) {
      std::cout << _commands_str << ": file as " << _consume_str.substr(1) << std::endl;

    } else if (_allow_stdout) {
      auto wait = false;
      for (auto &&value : stream) {
        if (std::exchange(wait, true)) {
          if (!linenoise("Next [Enter]")) {
            break;
          }
        }
        // todo: format value
        std::cout << value << std::endl;
      }
    } else {
      ignore_output();
    }
  }

 private:
  std::string_view _commands_str;
  std::string_view _consume_str;
  bool _allow_stdout;
};

auto parsePipelines(std::string_view line) -> ranges::any_view<Pipeline> {
  auto last_delim = line.find_last_of(';');

  if (last_delim == std::string_view::npos) {
    return ranges::views::single(Pipeline(line, true));
  }

  return ranges::views::concat(
      line.substr(0, last_delim) | ranges::views::split(';') |
          ranges::views::transform([&](auto &&s) { return Pipeline(toStringView(s), false); }),
      ranges::views::single(Pipeline(line.substr(last_delim + 1), true)));
}

// main.cpp

int main(int argc, char **argv) {
  for (const char *line; (line = linenoise("stream-shell v0.1 🚀> "));) {
    for (auto &&pipeline : parsePipelines(line)) {
      ranges::any_view<Value> stream = ranges::views::empty<Value>;
      for (auto &&cmd : pipeline.cmds()) {
        stream = cmd.run(stream);
      }
      pipeline.consume(stream);
    }
  }
  return 0;
}
