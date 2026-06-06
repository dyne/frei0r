# Using frei0r

Install frei0r first, then select effects from a host application or load them
programmatically. The host owns decoding, timelines, controls and output; the
plugin transforms or creates frames.

## Find plugins

On Unix-like systems, hosts search standard frei0r library directories such as:

- `/usr/lib/frei0r-1`
- `/usr/local/lib/frei0r-1`
- `$HOME/.frei0r-1/lib`

Set `FREI0R_PATH` to provide additional plugin directories:

```sh
export FREI0R_PATH="$HOME/my-frei0r:/opt/frei0r/lib/frei0r-1"
```

Unix uses `:` between entries. Windows uses `;`. The precise search and naming
rules are documented in
[`include/frei0r.h`](https://github.com/dyne/frei0r/blob/master/include/frei0r.h).

## FFmpeg

Apply a frei0r filter:

```sh
ffmpeg -i input.mp4 -vf "frei0r=filter_name=glow" output.mp4
```

Pass plugin parameters as a `|`-separated string:

```sh
ffmpeg -i input.mp4 \
  -vf "frei0r=filter_name=pixeliz0r:filter_params=0.08|0.08" \
  output.mp4
```

Use a generator plugin as a video source:

```sh
ffmpeg -f lavfi \
  -i "frei0r_src=size=1280x720:framerate=30:filter_name=partik0l" \
  -t 10 output.mp4
```

Plugin names and parameters differ by installation. A source checkout includes
a metadata scanner:

```sh
cd test
make frei0r-meta
make scan-meta
```

See the current
[FFmpeg frei0r documentation](https://ffmpeg.org/ffmpeg-filters.html#frei0r)
for complete option and escaping rules.

## Editors and frameworks

Kdenlive, Shotcut and Flowblade use MLT as their media engine. Installed frei0r
services appear through each editor's normal effects or filters interface.
Names and grouping can differ because the host supplies the user interface.

Liquidsoap exposes frei0r within programmable video pipelines. Start with its
[video documentation](https://www.liquidsoap.info/doc-dev/video.html).

The [supporting-software page](/software) distinguishes direct hosts from
applications receiving the plugins through another framework.

## Troubleshooting

1. Confirm the plugin package matches the host architecture.
2. Confirm the host was built with its frei0r integration enabled.
3. Check the standard plugin path or set `FREI0R_PATH`.
4. Inspect plugin metadata from a source build.
5. Ask on [Telegram](https://t.me/frei0r) or open a
   [GitHub issue](https://github.com/dyne/frei0r/issues) with operating system,
   host version, plugin name and installation path.
