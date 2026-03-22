#include "readline_impl.h"
#include "repl.h"

int main(int argc, char **argv) {
  ReadlinePromptImpl readline;
  return repl(readline), 0;
}
