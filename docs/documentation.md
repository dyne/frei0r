# Documentation

Choose the path that matches what you want to do.

<div class="resource-grid">
  <div class="resource-card"><h3><a href="./using-frei0r">Use frei0r</a></h3><p>Find installed plugins, use FFmpeg, and understand how applications expose effects.</p></div>
  <div class="resource-card"><h3><a href="./get-frei0r">Build and install</a></h3><p>Download releases, use package managers, or build the collection with CMake.</p></div>
  <div class="resource-card"><h3><a href="./develop-plugin">Write a plugin</a></h3><p>Implement the lifecycle, parameters and update function, then build and test it.</p></div>
  <div class="resource-card"><h3><a href="./api">Understand the API</a></h3><p>Learn the plugin types, color models and ABI before opening the complete generated reference.</p></div>
</div>

## Source of truth

- [`include/frei0r.h`](https://github.com/dyne/frei0r/blob/master/include/frei0r.h)
  is the normative C API and specification.
- [`include/frei0r.hpp`](https://github.com/dyne/frei0r/blob/master/include/frei0r.hpp)
  is the C++ convenience wrapper.
- [`src/filter/tutorial`](https://github.com/dyne/frei0r/tree/master/src/filter/tutorial)
  is the in-tree example.
- [Generated API reference](https://dyne.org/frei0r/codedoc/html/) provides a browsable rendering.

## Tutorials and demonstrations

Some linked videos show older software versions. They remain useful
demonstrations, not current installation instructions.

### Practical starting points

- [Use frei0r with FFmpeg](/using-frei0r#ffmpeg)
- [Develop a plugin from the in-tree tutorial](/develop-plugin)
- [Liquidsoap video documentation](https://www.liquidsoap.info/doc-dev/video.html)

### From the project story

These links accompany
[frei0r: the free and open source video effect preservation project](https://news.dyne.org/frei0r-the-free-and-open-source-video-effect-preservation-project/):

| Resource | Kind | Context |
| --- | --- | --- |
| [Liquidsoap video documentation](https://www.liquidsoap.info/doc-dev/video.html) | Current documentation | Use with the article's Anonymizer example; the original 2.0.6 tutorial URL is no longer available |
| [Liquidsoap - Anonymizer](https://www.youtube.com/watch?v=E7Fb0wV3h5Q) | Demonstration | A video anonymizer built with Liquidsoap and frei0r |
| [Liquidsoap video / OSC integration](https://www.youtube.com/watch?v=EX1PTjiuuXY) | Demonstration | Programmable video controlled over OSC |
| [Kdenlive 0.9.6 + Frei0r 14 Plugins](https://www.youtube.com/watch?v=xcFFo_bqg_4) | Historical tutorial | An older Kdenlive interface showing a range of effects |
| [frei0r demo](https://www.youtube.com/watch?v=yYB3rcfWpfw) | Demonstration | A short tour of visual effects |
| [The Art of the Algorithms](https://www.youtube.com/watch?v=5MexnBunH_g) | Historical context | Documentary on the demoscene culture behind many visual techniques |

Videos are linked rather than embedded so visiting this site does not
automatically contact YouTube.

For questions about adapting old examples to current software, join
[the frei0r discussion](https://t.me/frei0r).
