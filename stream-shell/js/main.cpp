#include <emscripten.h>
#include <emscripten/val.h>
#include "stream-shell/repl.h"

std::string buffer;

const char *readline(const char *prompt) {
  buffer = prompt;
  auto result = emscripten::val::global().call<emscripten::val>("readline", buffer).await();
  if (result.isNull()) {
    return nullptr;
  }
  buffer = result.as<std::string>();
  return buffer.c_str();
}

int main(int argc, char **argv) {
  return repl(readline), 0;
}
