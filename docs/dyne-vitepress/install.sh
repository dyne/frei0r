#!/usr/bin/env sh
set -eu

usage() {
  cat <<'EOF'
Usage: ./install.sh [--force] [--install] [docs-dir]

Copy the Dyne VitePress scaffold into a docs directory.

Options:
  --force    overwrite package, config, and starter content
  --install  run npm install in the docs directory after copying
  -h, --help show this help

If docs-dir is omitted, ./docs is used.
EOF
}

force=0
install=0
target="docs"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --force)
      force=1
      ;;
    --install)
      install=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    -*)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
    *)
      target="$1"
      ;;
  esac
  shift
done

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
template_dir="$script_dir/template"

copy_file() {
  src="$1"
  dst="$2"

  if [ "$force" -eq 1 ] || [ ! -e "$dst" ]; then
    mkdir -p "$(dirname -- "$dst")"
    cp "$src" "$dst"
    printf 'wrote %s\n' "$dst"
  else
    printf 'kept  %s\n' "$dst"
  fi
}

copy_tree_overwrite() {
  src_dir="$1"
  dst_dir="$2"

  mkdir -p "$dst_dir"
  cp -R "$src_dir"/. "$dst_dir"/
  printf 'synced %s\n' "$dst_dir"
}

mkdir -p "$target"

copy_file "$template_dir/package.json" "$target/package.json"
copy_file "$template_dir/index.md" "$target/index.md"
copy_file "$template_dir/about.md" "$target/about.md"
copy_file "$template_dir/guide.md" "$target/guide.md"
copy_file "$template_dir/.vitepress/config.mts" "$target/.vitepress/config.mts"

copy_tree_overwrite "$template_dir/.vitepress/theme" "$target/.vitepress/theme"
copy_tree_overwrite "$template_dir/public" "$target/public"

if [ "$install" -eq 1 ]; then
  (cd "$target" && npm install)
fi

cat <<EOF

Dyne VitePress scaffold installed in: $target

Next:
  cd "$target"
  npm install
  npm run dev
EOF

