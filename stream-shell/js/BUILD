load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")
load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")

cc_binary(
  name = "wasm-cc",
  srcs = [
    "main.cpp",
  ],
  deps = [
    "//stream-shell:stream-shell-lib",
  ],
  linkopts = [
    "-lembind",
    "-sASYNCIFY",
    "-sMODULARIZE",
    "-sEXPORT_ES6",
    "-sINCOMING_MODULE_JS_API=stdout,stderr",
  ],
)

wasm_cc_binary(
  name = "js",
  cc_target = ":wasm-cc",
)

refresh_compile_commands(
  name = "refresh_compile_commands",
  targets = {
    ":js": "",
  },
)
