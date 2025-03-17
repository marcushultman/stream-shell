cc_binary(
  name = "stream-shell",
  srcs = [
    "builtin.h",
    "command.h",
    "main.cpp",
    "pipeline.h",
    "value.h"
  ],
  deps = [
    "//util",
    "@protobuf//:json_util",
    "@protobuf//:struct_cc_proto",
    "@range-v3//:range-v3",
    "@linenoise.cpp//:linenoise-cpp"
  ],
)
