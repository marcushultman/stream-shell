cc_library(
  name = "stream-shell-lib",
  hdrs = [
    "builtin.h",
    "closure.h",
    "stream_parser.h",
    "stream_printer.h",
    "operand_op.h",
    "operand.h",
    "repl.h",
    "tokenize.h",
    "value_op.h",
    "variant_ext.h",
  ],
  srcs = [
    "stream_parser.cpp",
    "stream_printer.cpp",
    "tokenize.cpp",
  ],
  deps = [
    "//util",
    "@protobuf//:json_util",
    "@protobuf//:struct_cc_proto",
    "@range-v3//:range-v3",
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
