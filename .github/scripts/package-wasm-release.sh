#!/usr/bin/env bash

set -Eeuo pipefail

fatal() {
  printf 'error: %s\n' "$1" >&2
  exit 1
}

platform=${1:?platform required}
version=${2:?version required}
sdk_version=${3:?SDK version required}
build_dir=${4:?build directory required}
output_dir=${5:?output directory required}

[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] ||
  fatal "version must be SemVer without a leading v"
[[ "$sdk_version" =~ ^[0-9A-Za-z._+-]+$ ]] ||
  fatal "SDK version contains unsupported characters"
case "$platform" in
  wasi|emscripten) ;;
  *) fatal "platform must be wasi or emscripten" ;;
esac

command -v sha256sum >/dev/null || fatal "sha256sum is required"
command -v tar >/dev/null || fatal "tar is required"

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
source_dir=$(cd "$script_dir/../.." && pwd -P)
build_dir=$(cd "$build_dir" && pwd -P)
mkdir -p "$output_dir"
output_dir=$(cd "$output_dir" && pwd -P)

package_name="frei0r-${version}_${platform}"
package_dir="$output_dir/$package_name"
archive="$output_dir/${package_name}.tar.gz"
[[ ! -e "$package_dir" && ! -e "$archive" ]] ||
  fatal "package output already exists: $package_name"

mkdir -p "$package_dir/contracts" "$package_dir/include/frei0r" \
  "$package_dir/lib"
install -m 0644 "$source_dir/include/frei0r.h" \
  "$package_dir/include/frei0r.h"
install -m 0644 "$source_dir/include/frei0r/bundle.h" \
  "$package_dir/include/frei0r/bundle.h"
install -m 0644 "$build_dir/libfrei0r-bundle-static.a" \
  "$package_dir/lib/libfrei0r-bundle-static.a"

case "$platform" in
  wasi)
    sdk_name=wasi-sdk
    target=wasi-preview1
    runtime=node-wasi-preview1
    contract_files=(
      bundle-registry-wasi.wasm
      bundle-wasi-consumer.wasm
    )
    for file in "${contract_files[@]}"; do
      install -m 0644 "$build_dir/test/$file" "$package_dir/contracts/$file"
    done
    ;;
  emscripten)
    sdk_name=emsdk
    target=emscripten
    runtime=node
    contract_files=(
      bundle-registry-emscripten.js
      bundle-registry-emscripten.wasm
      bundle-emscripten-consumer.js
      bundle-emscripten-consumer.wasm
    )
    for file in "${contract_files[@]}"; do
      install -m 0644 "$build_dir/test/$file" "$package_dir/contracts/$file"
    done
    ;;
esac

{
  printf '{\n'
  printf '  "schema_version": 1,\n'
  printf '  "name": "frei0r-bundle",\n'
  printf '  "version": "%s",\n' "$version"
  printf '  "target": "%s",\n' "$target"
  printf '  "bundle_profile": "core",\n'
  printf '  "wasm_profile": "baseline-scalar-single-threaded",\n'
  printf '  "sdk": {"name": "%s", "version": "%s"},\n' \
    "$sdk_name" "$sdk_version"
  printf '  "runtime_test": "%s",\n' "$runtime"
  printf '  "library": "lib/libfrei0r-bundle-static.a",\n'
  printf '  "headers": ["include/frei0r.h", "include/frei0r/bundle.h"],\n'
  printf '  "tested_contracts": ['
  separator=
  for file in "${contract_files[@]}"; do
    printf '%s"contracts/%s"' "$separator" "$file"
    separator=', '
  done
  printf ']\n'
  printf '}\n'
} > "$package_dir/manifest.json"

(
  cd "$package_dir"
  find . -type f ! -name SHA256SUMS.txt -print0 |
    LC_ALL=C sort -z |
    xargs -0 sha256sum
) > "$package_dir/SHA256SUMS.txt"

tar -C "$output_dir" -czf "$archive" "$package_name"
printf '%s\n' "$archive"
