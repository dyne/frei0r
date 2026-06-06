# frei0r website maintenance

The website uses the Dyne-styled VitePress scaffold in `dyne-vitepress/` and
publishes generated Doxygen documentation in the same artifact.

## Requirements

- Node.js 24 or a compatible current LTS release
- npm
- Doxygen
- Graphviz

## Local development

```sh
cd docs
npm ci
npm run dev
```

Frontend-only builds do not require Doxygen:

```sh
npm run build:site
```

Use this while editing Markdown or the theme. Links under `/codedoc/html/` are
allowed because they are generated in the complete build.

## Complete production build

From `docs/`:

```sh
npm ci
npm run build
npm run check
```

`npm run build` runs `doxygen docs/Doxyfile` from the repository root, then
VitePress. Output is written to `docs/.vitepress/dist/`, including the API
reference under `codedoc/html/`.

Generated Doxygen output under `docs/public/codedoc/`, VitePress output,
caches, and `node_modules` are ignored.

## Preview

```sh
npm run preview
```

To test operation below a path prefix:

```sh
BASE_PATH=/frei0r/ npm run build:site
```

Production uses `/` because the custom domain is
`https://frei0r.dyne.org`.

## Content policy

Read `content-sources.md` before changing technical, historical, package or host
support claims:

1. The API header and current repository are authoritative.
2. Current host documentation supports integration claims.
3. The Dyne article and old sites support history and context.
4. Wikipedia supplies useful leads, not sole technical evidence.
5. Historical software must be labelled as historical.

Preserve the URL contract in `public-urls.md`.

## Deployment

`.github/workflows/pages.yml` validates pull requests and deploys `master` with
GitHub Pages actions. It installs Doxygen and Graphviz, executes the complete
build, checks the artifact, and deploys through the `github-pages` environment.

See `DEPLOYMENT.md` for Pages settings, custom-domain setup and production
verification.

## Template

`dyne-vitepress/template/` is the source scaffold. The website adapts its
`.vitepress` theme and public Dyne assets. Do not edit the template when a
change belongs only to frei0r.
