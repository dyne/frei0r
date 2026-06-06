# Public URL compatibility

The canonical website origin is <https://frei0r.dyne.org>. The production
VitePress base is `/`; preview builds may set `BASE_PATH`.

Last reviewed against `origin/gh_pages`: 2026-06-06.

## Retained URLs

| URL | Source in the new site | Reason |
| --- | --- | --- |
| `/` | `docs/index.md` | Canonical project landing page |
| `/codedoc/html/` | Generated from `docs/Doxyfile` | Existing public API reference |
| `/pics/fla_name_lb.webp` | `docs/public/pics/fla_name_lb.webp` | Existing frei0r wordmark URL |
| `/pics/frei0r.png` | `docs/public/pics/frei0r.png` | Existing social preview URL |
| `/frei0r-all.webm` | `docs/public/frei0r-all.webm` | Existing plugin demonstration |

The Doxygen build should preserve commonly linked generated files such as
`/codedoc/html/frei0r_8h.html` and `/codedoc/html/frei0r_8h_source.html`.
Generated filenames are not manually maintained; `include/frei0r.h` and the
Doxygen version determine them.

## Intentionally retired implementation assets

The old branch contains Bootstrap, jQuery, Popper, web fonts, and video-control
images under `/css/`, `/js/`, and `/img/`. They were private implementation
details of the one-page site, not project resources or documented public APIs.
The VitePress site does not copy them unless an inbound-link report identifies
a real compatibility requirement.

Old generated Doxygen assets are also not copied. They are replaced by a fresh
build from the current API header.

## Link rules

- Public Markdown links should be base-aware. Use relative links for VitePress
  pages and `withBase()` when custom components need a public asset URL.
- Canonical and social metadata use `https://frei0r.dyne.org`.
- Repository source links use `https://github.com/dyne/frei0r`.
- Do not link the old `gh_pages` branch as the current site.
- Keep `/codedoc/html/` available even though `/api` provides the reader-facing
  introduction.

## Cutover checks

After deployment, request every retained URL over HTTPS and verify:

1. The response is successful.
2. Assets have the expected media type.
3. VitePress page links stay on the configured base.
4. The Doxygen index and header pages load their own CSS, JavaScript, and images.
5. `frei0r.dyne.org` remains the canonical host.
