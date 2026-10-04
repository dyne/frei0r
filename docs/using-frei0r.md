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

## Use frei0r from an application

Applications can load individual plugin modules through the frei0r 1.2 ABI, or
link a bundle and select plugins from its registry. The bundle approach avoids
platform-specific dynamic loading and uses the same public descriptor API for
native and WebAssembly builds.

### C application

This complete example selects the `brightness` filter from a native bundle,
sets its normalized parameter, processes one RGBA frame and writes the raw
result to `frame.rgba`:

```c
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <frei0r/bundle.h>

enum { width = 64, height = 64, pixels = width * height };

int main(void)
{
  const f0r_plugin_descriptor_t *plugin;
  f0r_instance_t instance;
  _Alignas(16) uint32_t input[pixels];
  _Alignas(16) uint32_t output[pixels];
  unsigned char *rgba = (unsigned char *)input;
  double brightness = 0.75;
  FILE *file;
  int result = 0;
  int i;

  plugin = f0r_bundle_plugin_by_id("brightness");
  if (!plugin || plugin->descriptor_size != F0R_PLUGIN_DESCRIPTOR_SIZE ||
      plugin->descriptor_version != F0R_PLUGIN_DESCRIPTOR_VERSION ||
      !plugin->update)
    return 1;

  for (i = 0; i < pixels; ++i) {
    rgba[i * 4 + 0] = (unsigned char)(i * 3);  /* red */
    rgba[i * 4 + 1] = (unsigned char)(i * 5);  /* green */
    rgba[i * 4 + 2] = (unsigned char)(i * 7);  /* blue */
    rgba[i * 4 + 3] = 255;                     /* alpha */
  }
  memset(output, 0, sizeof(output));

  plugin->init();
  instance = plugin->construct(width, height);
  if (!instance) {
    plugin->deinit();
    return 2;
  }

  plugin->set_param_value(instance, &brightness, 0);
  plugin->update(instance, 0.0, input, output);

  file = fopen("frame.rgba", "wb");
  if (!file)
    result = 3;
  else if (fwrite(output, sizeof(output), 1, file) != 1)
    result = 4;
  plugin->destruct(instance);
  plugin->deinit();
  if (file && fclose(file) != 0 && result == 0)
    result = 5;
  return result;
}
```

Link it through the installed CMake package so the bundle's language runtime,
math library and optional dependency requirements are included in the correct
order:

```cmake
cmake_minimum_required(VERSION 3.12)
project(frei0r_app C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
find_package(Frei0rBundle CONFIG REQUIRED)
add_executable(frei0r-app app.c)
target_link_libraries(frei0r-app PRIVATE Frei0rBundle::static)
```

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/frei0r-bundle-prefix
cmake --build build
./build/frei0r-app
```

Frame dimensions must be positive multiples of eight, each row is tightly
packed, and frame storage must be aligned to 16 bytes. Inspect the plugin's
`f0r_plugin_info_t` before processing because a general host must support its
plugin type and color model. Parameters use normalized values unless their
declared type says otherwise. Keep the lifecycle paired: `init`, `construct`,
updates, `destruct`, then `deinit`.

### JavaScript application with WebAssembly

Release archives for Emscripten include the tested JavaScript loader and its
adjacent `.wasm` file under `contracts/`. The following Node application loads
that module and runs the same bounded application contract used in CI. It
processes three deterministic-noise frames with every bundled plugin and
varies supported numeric and color parameters:

```js
// app.mjs
import { createRequire } from 'node:module';
import { resolve } from 'node:path';

const modulePath = process.argv[2];
if (!modulePath) {
  console.error('usage: node app.mjs bundle-registry-emscripten.js');
  process.exit(64);
}

const require = createRequire(import.meta.url);
const createBundleApplication = require(resolve(modulePath));
const frei0r = await createBundleApplication();

const status = frei0r._f0r_bundle_emscripten_run();
if (status !== 0)
  throw new Error(`frei0r application failed with status ${status}`);

const frames = frei0r._f0r_bundle_emscripten_application_frame_count() >>> 0;
const changes = frei0r._f0r_bundle_emscripten_parameter_change_count() >>> 0;
const digest = frei0r._f0r_bundle_emscripten_output_digest() >>> 0;
console.log(`processed ${frames} frames with ${changes} parameter changes`);
console.log(`output digest: ${digest}`);
```

After extracting the matching release archive, keep the `.js` and `.wasm`
files together and pass the loader path to Node:

```sh
node app.mjs \
  ./frei0r-VERSION_emscripten/contracts/bundle-registry-emscripten.js
```

The contract module is an executable validation application, not a stable
general-purpose JavaScript processing API. To pass your own frames from
JavaScript, compile a small C ABI bridge against
`lib/libfrei0r-bundle-static.a`, export functions that own the plugin instance
and call its descriptor, then read and write those buffers through Emscripten's
typed-array views. The archive's `include/` directory contains the matching
`frei0r.h` and `frei0r/bundle.h`; do not mix headers and archives from different
releases or toolchains. The repository's
[Emscripten runner](https://github.com/dyne/frei0r/blob/master/test/run-bundle-registry-emscripten.mjs)
and
[C application contract](https://github.com/dyne/frei0r/blob/master/test/bundle-registry-contract.c)
are working bridge and host references.

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
cmake --build build --target frei0r-meta
cmake --build build --target generate-metadata
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
