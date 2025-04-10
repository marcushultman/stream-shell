#include "linenoise.h"
#include "repl.h"

int main(int argc, char **argv) {
  return repl(linenoise), 0;
}
