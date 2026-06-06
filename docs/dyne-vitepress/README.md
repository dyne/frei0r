# dyne-vitepress

Small scaffold for Dyne-styled VitePress documentation.

It is not a published VitePress theme package. It is a local installer that
copies the Dyne header, Dyne.org menu entry, theme CSS, and public assets into a
VitePress `docs/` directory. The starter content is lorem ipsum and can be
replaced by project docs.

## Use

```sh
./install.sh /path/to/project/docs
cd /path/to/project/docs
npm install
npm run dev
```

If the target docs directory is empty, the script initializes a minimal
VitePress site. If it already exists, the script refreshes the theme and assets
but leaves existing content and config files in place unless `--force` is used.

```sh
./install.sh --force /path/to/project/docs
```

`--force` overwrites the starter package, VitePress config, and lorem ipsum
content with the template versions.

## Layout

- `template/.vitepress/config.mts`: default VitePress config with the
  `About Dyne.org` navigation item
- `template/.vitepress/theme/`: Dyne-branded DefaultTheme extension
- `template/public/`: Dyne logo/logotype and favicon assets
- `template/*.md`: replaceable lorem ipsum content
- `man-to-md.pl`: converts `.TH`-based nroff manual pages from standard input
  to Markdown on standard output

