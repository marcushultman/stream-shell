cc_test(
  name = "test",
  deps = [
    "@boost.test",
    "//stream-shell:stream-shell-lib",
    "@protobuf//:differencer",
  ],
  srcs = [
    "config_test.cpp",
    "stream_parser_test.cpp",
    "test_env.h",
    "tokenize_test.cpp",
  ],
)

cc_binary(
  name = "linenoise-example",
  srcs = [
    "example.cpp",
  ],
  deps= [
    "@linenoise.cpp//:linenoise-cpp",
  ],
)
