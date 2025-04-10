#include <iostream>
#include <emscripten.h>
#include <emscripten/val.h>
#include "stream-shell/repl.h"

const char *linenoise(const char *prompt) {
  static std::string buffer;
  (std::cout << prompt).flush();
  auto result = emscripten::val::global().call<emscripten::val>("getline").await();
  if (result.isNull()) {
    return nullptr;
  }
  buffer = result.as<std::string>();
  return buffer.c_str();
}

int main(int argc, char **argv) {
  return repl(linenoise), 0;
}
