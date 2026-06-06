# History and preservation

frei0r grew from meetings between free video developers who wanted effects to
move between applications instead of being rewritten for every editor,
performance tool and media framework.

## From a broad proposal to a small common API

Around the Piksel gatherings in Bergen in the early 2000s, developers discussed
**LiViDO**, a broad video-plugin proposal. The participating projects had very
different internal architectures and could not agree that one ambitious API
should model all of them.

Two practical directions emerged:

- **frei0r** pursued a deliberately small interface for the common case:
  generators, one-input filters and mixers controlled by simple parameters.
- **WEED**, developed in connection with LiVES, retained a more featureful
  approach for richer host/plugin interaction.

The minimal approach made frei0r straightforward to adopt. It is often compared
to LADSPA in free audio software: not because the APIs are equivalent, but
because both established a small shared plugin contract around reusable signal
processing.

## A collection maintained across applications

Developers from Gephex, MLT, Kdenlive, FFmpeg, Liquidsoap, Pure Data, FreeJ,
LiVES and other projects contributed implementations and integrations. Effects
moved between command-line processing, non-linear editors, live performance and
programmable streaming.

The source tree became more than a plugin bundle. It preserves readable
algorithms for color, geometry, compositing, generators and experimental visual
processing. A maintained open implementation can outlive the application in
which an idea first appeared.

## Why preservation matters

Video-effect code is cultural and technical knowledge. When effects exist only
inside one abandoned application or proprietary plugin, their techniques become
hard to study and reuse. frei0r keeps a broad collection under free software
licenses and behind a stable, intentionally limited API.

That simplicity is a constraint, not an attempt to replace full media
frameworks. Hosts remain free to build richer timelines, interfaces, events and
automation around the shared effects.

## People and stewardship

frei0r is a collective effort coordinated through Dyne.org and maintained with
developers across its host ecosystem. The repository records the people and
changes more reliably than a static website:

- [`AUTHORS.md`](https://github.com/dyne/frei0r/blob/master/AUTHORS.md)
- [`ChangeLog`](https://github.com/dyne/frei0r/blob/master/ChangeLog)
- [GitHub contributors](https://github.com/dyne/frei0r/graphs/contributors)
- [Commit history](https://github.com/dyne/frei0r/commits/master/)

## External history and references

- [frei0r: the free and open source video effect preservation project](https://news.dyne.org/frei0r-the-free-and-open-source-video-effect-preservation-project/)
  is the canonical illustrated long-form project story, including interviews,
  software links and demonstrations.
- [Frei0r on Wikipedia](https://en.wikipedia.org/wiki/Frei0r) is a useful
  community-maintained overview and source trail. It includes pronunciation,
  early participants, the LiViDO/WEED context and a broad host list; current
  technical claims should still be checked against primary project sources.
- The [`gh_pages` branch](https://github.com/dyne/frei0r/tree/gh_pages)
  preserves the earlier frei0r website.

Continue with [supporting software](/software),
[tutorials](/documentation#tutorials-and-demonstrations), or the
[community page](/community).
