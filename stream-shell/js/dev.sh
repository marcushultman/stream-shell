#!/bin/bash

script_dir=$(dirname "$0")

GIT_REF=$(git symbolic-ref HEAD)
GIT_SHA=$(git rev-parse HEAD)

mkdir -p "$script_dir/dev"
cp "$script_dir/../../bazel-bin/stream-shell/js/js/wasm-cc.js" "$script_dir/dev/"
cp "$script_dir/../../bazel-bin/stream-shell/js/js/wasm-cc.wasm" "$script_dir/dev/"
cp "$script_dir/index.html" "$script_dir/dev/"
cp "$script_dir/main.css" "$script_dir/dev/"
sed "s|{{welcome_message}}|stream-shell $GIT_REF ($GIT_SHA)|" "$script_dir/main.js" > "$script_dir/dev/main.js"

chmod -x+w "$script_dir/dev/wasm-cc.js"
chmod -x+w "$script_dir/dev/wasm-cc.wasm"

cd "$script_dir/dev"

python3 -m http.server &
pid=$!

open "http://localhost:8000"

trap "kill ${pid}; exit 1" INT
wait
