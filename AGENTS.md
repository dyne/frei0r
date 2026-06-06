# Agent instructions

Prefer the simplest viable change. Keep code and documentation readable, avoid
new dependencies unless requested, and preserve existing user changes.

## Website

- Website source is under `docs/`; deployment is
  `.github/workflows/pages.yml`.
- `docs/dyne-vitepress/` is the local template reference, not public content.
- `include/frei0r.h` is authoritative for API facts.
- Review `docs/content-sources.md` before changing historical or ecosystem
  claims.
- Preserve the paths listed in `docs/public-urls.md`.
- Run `npm ci`, `npm run build`, and `npm run check` from `docs/` when Doxygen
  and Graphviz are installed.
- Use `npm run build:site` only for frontend iteration; it does not regenerate
  the API reference.
- Do not commit `node_modules`, `.vitepress` output, or generated Doxygen HTML.
- Keep GitHub and `https://t.me/frei0r` prominent.
- Label unverified or inactive host applications as historical.
