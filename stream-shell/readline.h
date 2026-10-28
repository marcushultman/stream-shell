#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <range/v3/view.hpp>

struct ReadlinePrompt {
  struct Result {
    std::string value;
    int trigger = {};
  };
  using Prompt = ranges::any_view<std::string>;
  using Triggers = std::unordered_set<int>;

  virtual ~ReadlinePrompt() = default;
  virtual std::optional<Result> prompt(Prompt, Triggers = {}) = 0;
  virtual void interrupt() = 0;
};
