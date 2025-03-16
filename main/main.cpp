#include "linenoise.h"
#include <iostream>

int main(int argc, char **argv) {
  const char *line;
  while ((line = linenoise("😀 \033[32mhello\x1b[0m> "))) {
    std::cout << line << std::endl;
  }
  return 0;
}
