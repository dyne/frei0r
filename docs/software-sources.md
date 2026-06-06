# Software integration inventory

This inventory backs the public software page. Status describes the evidence
available on 2026-06-06, not a compatibility guarantee for every package build.

## Current and verified

| Software | Relationship | Evidence |
| --- | --- | --- |
| [FFmpeg](https://ffmpeg.org/) | Direct host. Its current filter documentation includes the `frei0r` video filter and `frei0r_src` video source. Builds require frei0r support to be enabled. | [FFmpeg filter documentation](https://ffmpeg.org/ffmpeg-filters.html#frei0r) |
| [MLT](https://www.mltframework.org/) | Direct multimedia-framework host. MLT exposes frei0r services and is the integration layer used by several editors. | [MLT frei0r repository module](https://github.com/mltframework/mlt/tree/master/src/modules/frei0r) |
| [Kdenlive](https://kdenlive.org/) | Active editor built on MLT. Its effects documentation identifies the editor's effect workflow; frei0r services arrive through MLT. | [Kdenlive effects documentation](https://docs.kdenlive.org/en/effects_and_filters.html) |
| [Shotcut](https://www.shotcut.org/) | Active editor built on MLT; frei0r services arrive through that framework. | [Shotcut source](https://github.com/mltframework/shotcut) |
| [Flowblade](https://jliljebl.github.io/flowblade/) | Active editor built on MLT and distributed with frei0r plugins in its documented dependencies. | [Flowblade repository](https://github.com/jliljebl/flowblade) |
| [Liquidsoap](https://www.liquidsoap.info/) | Direct programmable media host with video operators backed by frei0r. | [Liquidsoap video documentation](https://www.liquidsoap.info/doc-dev/video.html) |

## Current projects requiring careful wording

These are useful ecosystem links, but the public page should avoid claiming a
current direct integration until its exact path is confirmed.

| Software | Safe description |
| --- | --- |
| [GStreamer](https://gstreamer.freedesktop.org/) | Has carried frei0r plugin wrappers; verify the currently shipped plugin set for the linked release. |
| [Pure Data](https://puredata.info/) | Historical/current multimedia environment associated with frei0r wrappers; link a maintained wrapper before calling it direct support. |
| [OpenShot](https://www.openshot.org/) | Active editor using FFmpeg and historically listed as a frei0r host; describe only after verifying current frei0r exposure. |
| [Pitivi](https://www.pitivi.org/) | Active GStreamer editor historically listed as a host; any frei0r availability is through its multimedia stack. |
| [gmerlin](https://gmerlin.sourceforge.net/) | Multimedia framework historically connected to frei0r and gavl; current project status and integration need verification. |

## Historical adopters

These projects matter to frei0r's interoperability and preservation history,
but should not be presented as recommended current hosts without new evidence.

- [LiVES](https://sourceforge.net/projects/lives/)
- [FreeJ](https://freej.dyne.org/)
- Gephex
- [Open Movie Editor](https://sourceforge.net/projects/openmovieeditor/)
- [VeeJay](https://veejayhq.github.io/)
- MøB
- DVEdit
- AVconv/Libav

Where a project site is unavailable, the public history should link a reliable
archive or omit the link rather than sending readers to an unrelated domain.

## Editorial rules

- "Supports frei0r" means a current primary source identifies a direct loader or
  wrapper.
- "Through MLT" and "through GStreamer" are integration paths, not separate
  direct implementations.
- Do not infer current support from Wikipedia or the old frei0r website alone.
- Put current, well-documented choices before historical projects.
- Link to the host's frei0r-specific documentation when one exists.
