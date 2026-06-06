# Agent Instructions

This repository is a Dyne-styled VitePress template and conversion workspace.
Keep changes simple, reversible, and local to the requested site.

## Core Rules

- Do not commit anything from this repository unless the user explicitly asks.
- Preserve existing content first. Reuse pages, images, metadata, links, and
  domain files by reorganizing them into the VitePress site.
- Do not delete or move legacy HTML pages unless the user explicitly asks.
- Preserve public URLs for existing HTML pages. When converting a site, copy
  legacy root HTML pages into the VitePress `public/` directory so they still
  build at root, such as `/old-page.html`.
- When a site may be served from a subpath, such as `/hasciicam/`, use relative
  links in converted markdown where practical and build with
  `BASE_PATH=/subpath/ npm run build`.
- Normalize legacy internal links that point to an old absolute site root, such
  as `https://ascii.dyne.org`, into relative links before copying them to
  `public/`.
- Prefer the template already present in this repository over introducing new
  dependencies or a new theme.
- Keep generated and dependency folders out of normal versioning:
  `node_modules/`, `.vitepress/dist/`, `.vitepress/cache/`, and local build
  artifacts should be ignored in the target site when appropriate.

## Template Source

Use `template/` as the canonical VitePress source:

- `template/.vitepress/` for config, theme, components, and styling.
- `template/public/` for shared Dyne assets.
- `template/package.json` and `template/package-lock.json` for the VitePress
  dependency setup.
- `template/index.md`, `template/guide.md`, and `template/about.md` only as
  content shape examples, not as final copy.

Do not copy `template/node_modules/` or `template/.vitepress/dist/`.

## Creating A Site In An Indicated Subdirectory

When the user points to a subdirectory, create the VitePress website inside that
subdirectory without versioning it from this root repository.

1. Inspect the subdirectory before editing.
2. Copy the template VitePress setup into the subdirectory:
   - `.vitepress/`
   - `package.json`
   - `package-lock.json`
   - shared files from `template/public/`
3. Copy existing static assets into the subdirectory's `public/` tree.
4. Copy existing root HTML pages into `public/` to preserve their output paths.
5. Replace only the root `index.html` when converting it into VitePress
   `index.md`.
6. Convert the original index content into a markdown page when requested, often
   as `usage-guide.md` or another user-provided name.
7. Update `.vitepress/config.mts` with the site's real title, description,
   nav, social links, and sidebar.
8. Run `npm install` and `npm run build` from the target subdirectory. If the
   site is served from a subpath, run the build with `BASE_PATH=/subpath/`.
9. Verify generated root paths in `.vitepress/dist/`, especially legacy HTML
   pages and images.

## Creating A Site In The Parent

When the user wants this template applied to a parent or external directory,
treat this repository as the source template and the parent as the target.

1. Inspect the parent target first.
2. Copy only the template files needed for a VitePress site.
3. Preserve the target's existing content and public URLs.
4. Reorganize existing target information into VitePress markdown and
   `public/`, keeping legacy HTML pages at their existing output paths.
5. Build and verify from the parent target.
6. Do not commit in either repository unless the user explicitly asks.

## Content Conversion

- Main landing pages should normally become VitePress `index.md` with a hero,
  a concise description, primary actions, and feature cards.
- Preserve detailed legacy index content as a markdown guide page instead of
  discarding it.
- Keep external download/source links intact unless the user supplies new ones.
- Keep old images and captions meaningful; prefer real project imagery already
  present in the source site.
- Keep wording faithful to the existing site, but clean obvious typos and
  obsolete presentation markup when converting to markdown.

## Publishing Manual Pages

When the source project contains nroff man pages, use the bundled
`man-to-md.pl` converter instead of transcribing them by hand. The script reads
one man page from standard input and writes Markdown to standard output:

```sh
perl /path/to/dyne-vitepress/man-to-md.pl < path/to/tool.1 > tool.1.md
```

- Run the command once per man page and place the generated `.md` files in the
  target VitePress content tree.
- Keep the man page as the source of truth. Regenerate the Markdown after the
  source changes; do not maintain divergent prose in both files.
- Use `-c` to mark generated output when useful:
  `perl /path/to/dyne-vitepress/man-to-md.pl -c < tool.1 > tool.1.md`.
- The converter supports nroff man macros beginning with `.TH`. It rejects mdoc
  pages beginning with `.Dd` or `.Dt`; convert those with an appropriate
  external tool or document the unsupported input instead of rewriting the
  source.
- Inspect the generated Markdown for headings, synopsis blocks, lists, links,
  and escaped characters before publishing.
- Add published manual pages to the VitePress nav or sidebar when appropriate,
  then run the normal build validation.
- Run `perl man-to-md.pl --help` from this repository to see optional section
  insertion, formatting, title-casing, and dash-handling flags.

## Validation

After changes, run from the target site:

```sh
npm install
npm run build
```

Then verify:

- `.vitepress/dist/index.html` exists.
- Converted markdown pages render as expected.
- Legacy HTML pages still exist at `.vitepress/dist/<page>.html`.
- Legacy assets referenced by those pages exist under `.vitepress/dist/`.
- Subpath builds prefix VitePress URLs with the configured base, and copied
  legacy HTML pages use relative links that remain under that subpath.
- `git status` shows no accidental commits and no unexpected tracked changes.
