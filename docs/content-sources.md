# Website content sources

This file is the editorial source map for the frei0r website. It is not part of
the public navigation.

Last reviewed: 2026-06-06.

## Authority order

Use the first applicable source in this order:

1. `include/frei0r.h` for the API contract.
2. Current repository code, build files, tests, and GitHub workflows.
3. GitHub releases and current official documentation from host projects.
4. `README.md`, `BUILD.md`, `AUTHORS.md`, and `ChangeLog`.
5. Historical Dyne pages and the `gh_pages` branch.
6. Wikipedia and other secondary sources as research leads only.

Technical or current-status claims from levels 5 and 6 need corroboration from
a primary source.

## Repository sources

| Source | Use |
| --- | --- |
| [`include/frei0r.h`](https://github.com/dyne/frei0r/blob/master/include/frei0r.h) | Normative API 1.2 specification, ABI, plugin types, color models, parameters, paths, and icons |
| [`include/frei0r.hpp`](https://github.com/dyne/frei0r/blob/master/include/frei0r.hpp) | C++ convenience wrapper |
| [`src/filter/tutorial`](https://github.com/dyne/frei0r/tree/master/src/filter/tutorial) | Maintained plugin example |
| [`README.md`](https://github.com/dyne/frei0r/blob/master/README.md) | Project summary, community, downloads, and packaging |
| [`BUILD.md`](https://github.com/dyne/frei0r/blob/master/BUILD.md) | Build and test commands |
| [`AUTHORS.md`](https://github.com/dyne/frei0r/blob/master/AUTHORS.md) | Maintainers and contributors |
| [`ChangeLog`](https://github.com/dyne/frei0r/blob/master/ChangeLog) | Release history |
| [`CMakeLists.txt`](https://github.com/dyne/frei0r/blob/master/CMakeLists.txt) | Version and build options |
| [GitHub releases](https://github.com/dyne/frei0r/releases) | Current source and binary releases |

## Historical sources

| Source | Use |
| --- | --- |
| [`origin/gh_pages`](https://github.com/dyne/frei0r/tree/gh_pages) | Existing domain, assets, demo video, and `/codedoc/html/` URL |
| [Dyne software page](https://dyne.org/software/frei0r/) | Earlier project description and history |
| [Preservation article](https://news.dyne.org/frei0r-the-free-and-open-source-video-effect-preservation-project/) | Canonical long-form history, motivation, demonstrations, and tutorials |
| [Wikipedia](https://en.wikipedia.org/wiki/Frei0r) | Secondary ecosystem and history cross-check; useful leads include LiViDO, WEED, the LADSPA analogy, pronunciation, and host applications |

Paraphrase historical prose and link its source. Do not reproduce long passages.

## Current external sources

- [FFmpeg frei0r filter and source](https://ffmpeg.org/ffmpeg-filters.html#frei0r)
- [MLT](https://www.mltframework.org/)
- [Kdenlive](https://kdenlive.org/)
- [Shotcut](https://www.shotcut.org/)
- [Flowblade](https://jliljebl.github.io/flowblade/)
- [Liquidsoap video documentation](https://www.liquidsoap.info/doc-dev/video.html)
- [Pure Data](https://puredata.info/)
- [GStreamer](https://gstreamer.freedesktop.org/)
- [Repology](https://repology.org/project/frei0r/versions)
- [Homebrew](https://formulae.brew.sh/formula/frei0r)
- [FreeBSD FreshPorts](https://www.freshports.org/graphics/frei0r/)
- [NetBSD pkgsrc](https://pkgsrc.se/multimedia/frei0r)

Host applications must be described conservatively. Verify direct frei0r
support in the host's current documentation or source. When an editor receives
frei0r through MLT or FFmpeg, say so instead of implying a separate integration.

## Publishing sources

- [VitePress deployment guide](https://vitepress.dev/guide/deploy)
- [GitHub Pages custom workflows](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages)
- [GitHub Pages custom domains](https://docs.github.com/en/pages/configuring-a-custom-domain-for-your-github-pages-site/managing-a-custom-domain-for-your-github-pages-site)
- [Doxygen configuration manual](https://www.doxygen.nl/manual/config.html)

## Excluded stale material

Do not publish these as current instructions or services:

- Autotools and Cygwin build instructions superseded by the repository's CMake
  build.
- Zeranoe FFmpeg builds.
- Freecode and OpenHub project-status widgets.
- Old plugin totals copied from static pages.
- Dead host-project URLs or unverified claims that every historical host remains
  maintained.
- The old generic Dyne chat links as a replacement for the project discussion
  link at <https://t.me/frei0r>.

When historical software is worth preserving, label it "historical" and link
an archive or surviving project page.
