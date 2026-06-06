# Software supporting frei0r

frei0r is useful because effects are shared between hosts. Support may be
direct, or provided by a multimedia framework such as MLT.

## Current, verified hosts

<div class="software-grid">
  <div class="software-card"><span class="status-label">Direct host</span><h3><a href="https://ffmpeg.org/">FFmpeg</a></h3><p>Provides the <code>frei0r</code> filter and <code>frei0r_src</code> source in filter graphs. See the <a href="https://ffmpeg.org/ffmpeg-filters.html#frei0r">official filter documentation</a>.</p></div>
  <div class="software-card"><span class="status-label">Direct framework host</span><h3><a href="https://www.mltframework.org/">MLT</a></h3><p>Loads frei0r services for applications through its maintained <a href="https://github.com/mltframework/mlt/tree/master/src/modules/frei0r">frei0r module</a>.</p></div>
  <div class="software-card"><span class="status-label">Through MLT</span><h3><a href="https://kdenlive.org/">Kdenlive</a></h3><p>A full non-linear editor whose effect system is built on MLT services.</p></div>
  <div class="software-card"><span class="status-label">Through MLT</span><h3><a href="https://www.shotcut.org/">Shotcut</a></h3><p>A cross-platform editor built on MLT, exposing effects through its filter interface.</p></div>
  <div class="software-card"><span class="status-label">Through MLT</span><h3><a href="https://jliljebl.github.io/flowblade/">Flowblade</a></h3><p>A GNU/Linux non-linear editor whose documented dependencies include MLT and frei0r plugins.</p></div>
  <div class="software-card"><span class="status-label">Direct host</span><h3><a href="https://www.liquidsoap.info/">Liquidsoap</a></h3><p>A programmable streaming language with video operators backed by frei0r. See its <a href="https://www.liquidsoap.info/doc-dev/video.html">video documentation</a>.</p></div>
</div>

## Other integration paths

These projects have current or historical connections to frei0r, but plugin
availability can depend on the installed distribution, wrapper, or multimedia
stack:

- **GStreamer** has shipped frei0r wrappers in its plugin collections.
- **Pure Data** has been used with frei0r through video and plugin wrappers.
- **Pitivi** uses GStreamer; frei0r availability depends on that stack.
- **OpenShot** uses FFmpeg and has historically been listed among frei0r hosts.
- **gmerlin** and gavl share part of frei0r's free-video ecosystem.

Check the current host and operating-system package documentation before
depending on one of these paths.

## Historical adopters

Earlier free video projects helped shape, test and spread the common plugin
interface. They remain part of the project's history even when they are no
longer a practical recommendation:

- [LiVES](https://sourceforge.net/projects/lives/)
- [FreeJ](https://freej.dyne.org/)
- Gephex
- [Open Movie Editor](https://sourceforge.net/projects/openmovieeditor/)
- [VeeJay](https://veejayhq.github.io/)
- MøB
- DVEdit
- AVconv/Libav

The [project history](/history) explains how application developers arrived at
the minimal shared API.

## Host developers

The normative interface is the small
[`include/frei0r.h`](https://github.com/dyne/frei0r/blob/master/include/frei0r.h)
header. A host discovers shared libraries, reads plugin metadata, constructs an
instance and calls the update function appropriate to the plugin type.

- [Read the API overview](/api)
- [Browse generated API documentation](/codedoc/html/)
- [Discuss integrations](https://t.me/frei0r)
