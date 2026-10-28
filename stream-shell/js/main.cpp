#include <emscripten.h>
#include <emscripten/val.h>
#include "stream-shell/readline.h"
#include "stream-shell/repl.h"

struct ReadlinePromptJSImpl : public ReadlinePrompt {
  std::optional<Result> prompt(Prompt prompt, Triggers triggers) override {
    auto prompts = (prompt | ranges::views::take(1) | ranges::to<std::vector>);
    auto prompt0 = prompts.empty() ? "" : prompts.front();

    auto result = emscripten::val::global().call<emscripten::val>("readline", prompt0).await();

    if (result.isNull()) {
      return {};
    }
    return Result{.value = result.as<std::string>()};
  }

  void interrupt() override {}
};

int main(int argc, char **argv) {
  ReadlinePromptJSImpl readline;
  return repl(readline), 0;
}
