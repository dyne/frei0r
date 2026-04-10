#!/usr/bin/env bash

set -euo pipefail

base_sha=${1:?base sha required}
head_sha=${2:?head sha required}

mode=full
reason="changed files require the full suite"
declare -a plugin_dirs=()
declare -a targets=()
declare -a plugin_paths=()

fatal() {
  printf 'error: %s\n' "$1" >&2
  exit 1
}

is_plugin_tree() {
  case "$1" in
    src/filter/*|src/mixer2/*|src/mixer3/*) return 0 ;;
    *) return 1 ;;
  esac
}

plugin_dir_from_path() {
  local path=$1
  IFS=/ read -r top category name _ <<< "$path"
  if [[ "$top" == "src" && -n "${category:-}" && -n "${name:-}" ]]; then
    printf '%s/%s/%s\n' "$top" "$category" "$name"
    return 0
  fi
  return 1
}

contains_dir() {
  local needle=$1
  shift || true
  local item
  for item in "$@"; do
    if [[ "$item" == "$needle" ]]; then
      return 0
    fi
  done
  return 1
}

dir_exists_in_commit() {
  local commit=$1
  local path=$2
  git cat-file -e "${commit}:${path}" 2>/dev/null
}

parent_cmake_registers_plugin() {
  local commit=$1
  local plugin_dir=$2
  local category=${plugin_dir#src/}
  category=${category%%/*}
  local plugin_name=${plugin_dir##*/}
  local parent_cmake="src/${category}/CMakeLists.txt"

  local parent_text
  parent_text=$(git show "${commit}:${parent_cmake}" 2>/dev/null) || {
    return 1
  }

  printf '%s\n' "$parent_text" | grep -Eq "^[[:space:]]*add_subdirectory[[:space:]]*\\([[:space:]]*${plugin_name}[[:space:]]*\\)"
}

extract_target_name() {
  local commit=$1
  local plugin_dir=$2
  local plugin_name=${plugin_dir##*/}
  local cmake_file="${plugin_dir}/CMakeLists.txt"

  local cmake_text
  cmake_text=$(git show "${commit}:${cmake_file}" 2>/dev/null) || {
    return 1
  }

  local target
  target=$(printf '%s\n' "$cmake_text" | sed -nE 's/^[[:space:]]*set[[:space:]]*\([[:space:]]*TARGET[[:space:]]+([^[:space:])]+).*/\1/p' | head -n1)
  if [[ -n "$target" ]]; then
    printf '%s\n' "$target"
    return 0
  fi

  target=$(printf '%s\n' "$cmake_text" | sed -nE 's/^[[:space:]]*add_library[[:space:]]*\(([[:alnum:]_.+-]+)[[:space:]]+MODULE.*/\1/p' | head -n1)
  if [[ -n "$target" ]]; then
    printf '%s\n' "$target"
    return 0
  fi

  printf '%s\n' "$plugin_name"
}

while IFS=$'\t' read -r status path new_path; do
  [[ -n "${status:-}" ]] || continue

  if [[ "$status" == A* ]] && is_plugin_tree "$path"; then
    plugin_dir=$(plugin_dir_from_path "$path") || true
    if [[ -n "${plugin_dir:-}" ]] && dir_exists_in_commit "$base_sha" "$plugin_dir"; then
      reason="PR adds files inside existing plugin directory: $plugin_dir"
      plugin_dirs=()
      break
    fi

    if [[ -n "${plugin_dir:-}" ]] && ! contains_dir "$plugin_dir" "${plugin_dirs[@]}"; then
      plugin_dirs+=("$plugin_dir")
    fi
    continue
  fi

  case "$path" in
    src/filter/CMakeLists.txt|src/mixer2/CMakeLists.txt|src/mixer3/CMakeLists.txt)
      continue
      ;;
    *)
      reason="PR touches non-new-plugin files: $path"
      plugin_dirs=()
      break
      ;;
  esac
done < <(git diff --name-status --find-renames "$base_sha" "$head_sha")

if [[ ${#plugin_dirs[@]} -gt 0 ]]; then
  mode=targeted
  reason="PR only adds new plugin directories"

  for plugin_dir in "${plugin_dirs[@]}"; do
    if ! parent_cmake_registers_plugin "$head_sha" "$plugin_dir"; then
      fatal "parent CMakeLists.txt does not register ${plugin_dir}. Add add_subdirectory(${plugin_dir##*/}) to the category CMakeLists.txt."
    fi

    target=$(extract_target_name "$head_sha" "$plugin_dir") || {
      mode=full
      reason="could not resolve target for $plugin_dir"
      targets=()
      plugin_paths=()
      break
    }

    plugin_paths+=("build/${plugin_dir}/${target}.so")
    targets+=("$target")
  done
fi

printf 'mode=%s\n' "$mode"
printf 'reason=%s\n' "$reason"

if [[ "$mode" == "targeted" ]]; then
  {
    printf 'targets<<EOF\n'
    printf '%s\n' "${targets[*]}"
    printf 'EOF\n'
    printf 'plugin_paths<<EOF\n'
    printf '%s\n' "${plugin_paths[*]}"
    printf 'EOF\n'
    printf 'plugin_dirs<<EOF\n'
    printf '%s\n' "${plugin_dirs[*]}"
    printf 'EOF\n'
  }
else
  {
    printf 'targets=\n'
    printf 'plugin_paths=\n'
    printf 'plugin_dirs=\n'
  }
fi
