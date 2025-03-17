#include <range/v3/all.hpp>
#include "linenoise.h"
#include "pipeline.h"
#include "value.h"

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
