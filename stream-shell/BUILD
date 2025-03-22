cc_library(
  name = "stream-shell-lib",
  hdrs = [
    "builtin.h",
    "command.h",
    "pipeline.h",
    "tokenize.h",
    "value.h"
  ],
  srcs = [
    "tokenize.cpp",
  ],
  deps = [
    "//util",
    "@protobuf//:json_util",
    "@protobuf//:struct_cc_proto",
    "@range-v3//:range-v3",
    "@linenoise.cpp//:linenoise-cpp"
  ],
  visibility = ["//stream-shell:__subpackages__"],
)

cc_binary(
  name = "stream-shell",
  srcs = [
    "main.cpp",
  ],
  deps = [
    "@linenoise.cpp//:linenoise-cpp",
    "stream-shell-lib"
  ],
)
