#pragma once

#include <future>
#include <optional>
#include <string>
#include <string_view>

struct ReadlinePrompt {
  virtual ~ReadlinePrompt() = default;
  virtual std::future<std::optional<std::string>> prompt(std::string_view prompt) = 0;
  virtual void refresh(std::string_view prompt) = 0;
  virtual void interrupt() = 0;
};
