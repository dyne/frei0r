#!/usr/bin/env bash

set -Eeuo pipefail

packager=${1:?packager path required}
source_dir=${2:?source directory required}
work_dir=${3:?work directory required}

mkdir -p "$work_dir"
test_root=$(mktemp -d "$work_dir/wasm-release-package-test.XXXXXX")
trap 'rm -rf -- "$test_root"' EXIT

make_fixture() {
  local platform=$1
  local build_dir="$test_root/build-$platform"
  mkdir -p "$build_dir/test"
  printf 'archive-%s\n' "$platform" > "$build_dir/libfrei0r-bundle-static.a"
  case "$platform" in
    wasi)
      printf 'registry\n' > "$build_dir/test/bundle-registry-wasi.wasm"
      printf 'consumer\n' > "$build_dir/test/bundle-wasi-consumer.wasm"
      ;;
    emscripten)
      printf 'registry-js\n' > "$build_dir/test/bundle-registry-emscripten.js"
      printf 'registry-wasm\n' > "$build_dir/test/bundle-registry-emscripten.wasm"
      printf 'consumer-js\n' > "$build_dir/test/bundle-emscripten-consumer.js"
      printf 'consumer-wasm\n' > "$build_dir/test/bundle-emscripten-consumer.wasm"
      ;;
  esac
}

verify_package() {
  local platform=$1
  local package_name="frei0r-9.8.7_${platform}"
  local archive="$test_root/dist-$platform/${package_name}.tar.gz"
  local extract_dir="$test_root/extract-$platform"

  "$packager" "$platform" 9.8.7 test-sdk \
    "$test_root/build-$platform" "$test_root/dist-$platform"
  [[ -f "$archive" ]]
  mkdir -p "$extract_dir"
  tar -xzf "$archive" -C "$extract_dir"
  (
    cd "$extract_dir/$package_name"
    sha256sum -c SHA256SUMS.txt
  )
  local manifest="$extract_dir/$package_name/manifest.json"
  grep -Fq "\"target\":" "$manifest"
  grep -Fq '"input": "deterministic-noise"' "$manifest"
  grep -Fq '"frames_per_plugin": 3' "$manifest"
  grep -Fq '"parameters": "varied"' "$manifest"
  [[ -f "$extract_dir/$package_name/include/frei0r.h" ]]
  [[ -f "$extract_dir/$package_name/include/frei0r/bundle.h" ]]
  [[ -f "$extract_dir/$package_name/lib/libfrei0r-bundle-static.a" ]]
}

[[ -f "$source_dir/include/frei0r.h" ]]
make_fixture wasi
make_fixture emscripten
verify_package wasi
verify_package emscripten
