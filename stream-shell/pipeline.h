#pragma once

#include <algorithm>
#include <iostream>
#include <ranges>
#include <range/v3/all.hpp>
#include "command.h"
#include "linenoise.h"
#include "util/to_string_view.h"

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
