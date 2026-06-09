---
layout: home

hero:
  name: "frei0r"
  text: "Free video effects, shared"
  tagline: "A minimalistic plugin API and a portable collection of free and open source filters, generators and mixers."
  image:
    src: /pics/fla_name_lb.webp
    alt: frei0r
  actions:
    - theme: brand
      text: Explore on GitHub
      link: https://github.com/dyne/frei0r
    - theme: alt
      text: Join the discussion
      link: https://t.me/frei0r
    - theme: alt
      text: Get frei0r
      link: /get-frei0r

features:
  - title: "Portable"
    details: "Small C and C++ plugins run on GNU/Linux, macOS and Windows without special video hardware."
  - title: "Interoperable"
    details: "One deliberately simple API lets effects travel between editors, command-line tools and media frameworks."
  - title: "Preserved"
    details: "Readable source keeps video-effect algorithms available as reusable knowledge, not locked inside one application."
  - title: "Community-built"
    details: "Contributors have maintained and expanded the collection across two decades of free video software."
---

## What frei0r is

frei0r is both a **minimalistic video-effect plugin API** and a large collection
of plugins implementing that API. A host gives a plugin one or more video
frames; the plugin transforms, mixes or generates frames and returns the result.

The deliberately narrow contract makes effects easy to implement, inspect and
reuse. It has played a role for free video software comparable to the role
LADSPA played for audio plugins: a small common interface that avoids repeatedly
reimplementing useful algorithms.

<div class="frei0r-stats">
  <div class="frei0r-stat"><strong>100+</strong> video-effect plugins</div>
  <div class="frei0r-stat"><strong>4</strong> plugin types: source, filter, mixer2 and mixer3</div>
  <div class="frei0r-stat"><strong>3</strong> major desktop platforms</div>
</div>

### What it is not

frei0r is not a complete video application or a general framework for every
kind of media plugin. It intentionally omits complex event, timeline and UI
systems. Applications and frameworks such as FFmpeg and MLT provide those
higher-level capabilities around the effects.

## Use frei0r in your software

frei0r effects are available through widely used free software:

<div class="software-grid">
  <div class="software-card"><span class="status-label">Direct host</span><h3><a href="https://ffmpeg.org/">FFmpeg</a></h3><p>Apply filters or create video sources from the command line and filter graphs.</p></div>
  <div class="software-card"><span class="status-label">Framework</span><h3><a href="https://www.mltframework.org/">MLT</a></h3><p>Loads frei0r services for editors and other multimedia applications.</p></div>
  <div class="software-card"><span class="status-label">Video editor</span><h3><a href="https://kdenlive.org/">Kdenlive</a></h3><p>Uses MLT and makes frei0r effects available in an editing workflow.</p></div>
  <div class="software-card"><span class="status-label">Video editor</span><h3><a href="https://www.shotcut.org/">Shotcut</a></h3><p>Uses MLT to provide a cross-platform editing and effects environment.</p></div>
  <div class="software-card"><span class="status-label">Video editor</span><h3><a href="https://jliljebl.github.io/flowblade/">Flowblade</a></h3><p>Integrates MLT services in a GNU/Linux non-linear editor.</p></div>
  <div class="software-card"><span class="status-label">Programmable media</span><h3><a href="https://www.liquidsoap.info/">Liquidsoap</a></h3><p>Uses frei0r in programmable audio and video streams.</p></div>
</div>

[See the full supporting-software list →](/software)

## An open collection of visual knowledge

The source is useful beyond the plugins themselves. It is a library of readable
formulas for color correction, compositing, geometry, blur, distortion,
analysis, generators and experimental image processing.

<div class="project-callout">
  <strong>The preservation story</strong>
  <p>Read <a href="https://news.dyne.org/frei0r-the-free-and-open-source-video-effect-preservation-project/">frei0r: the free and open source video effect preservation project</a> for the project’s origins, people, software ecosystem and demonstrations.</p>
</div>

## See the collection in motion

The following rapid demonstration contains flashing and high-contrast imagery.

<video class="demo-video" controls preload="metadata" src="/frei0r-all.webm">
  Your browser cannot play the demo.
  <a href="/frei0r-all.webm">Download the frei0r demonstration video.</a>
</video>

## Take part

- [Download a release and install frei0r](/get-frei0r)
- [Use frei0r from an application or FFmpeg](/using-frei0r)
- [Develop and contribute a plugin](/develop-plugin)
- [Browse the API documentation](/api)
- [Browse the generated API reference](/codedoc/html/)
- [Discuss the project on Telegram](https://t.me/frei0r)
- [Report issues and send patches on GitHub](https://github.com/dyne/frei0r)
