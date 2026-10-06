# Live demo maintenance

The public demo is <https://dyne.org/frei0r/demo/>. It is a separate Svelte
application that is composed into the VitePress Pages artifact; it is not a
VitePress page. Its source is [`demo/`](demo/), its browser runtime is built
from [`../examples/browser-demo/runtime/`](../examples/browser-demo/runtime/),
and its public artifact is `docs/.vitepress/dist/demo/`.

## Toolchain

Use Node.js 24 and npm, CMake, Ninja, Doxygen, Graphviz, and Emscripten 4.0.1.
The Pages workflow in [`../.github/workflows/pages.yml`](../.github/workflows/pages.yml)
is the pinned reference environment. Keep Emscripten's cache outside the SDK
installation and outside source-controlled paths.

Install website dependencies and the Chromium package used by browser smoke
coverage:

```sh
cd docs
npm ci
npx playwright install chromium chromium-headless-shell
```

On Linux, use `npx playwright install --with-deps chromium chromium-headless-shell`
when the browser's system libraries are not already available. CI uses that
form. Do not commit `node_modules`, the Emscripten cache, generated Doxygen
HTML, VitePress output, or browser runtime products.

## Clean Pages-equivalent build

From the repository root, activate Emscripten 4.0.1 and build the explicit
browser runtime in a fresh directory:

```sh
export EMSDK=/path/to/emsdk
source "$EMSDK/emsdk_env.sh"
export EM_CACHE="$PWD/build/demo-emscripten/em-cache"
cmake -S . -B build/demo-emscripten -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
  -DFREI0R_BUILD_BROWSER_DEMO_RUNTIME=ON \
  -DBUILD_TESTING=ON \
  -DWITHOUT_OPENCV=ON \
  -DWITHOUT_CAIRO=ON \
  -DWITHOUT_GAVL=ON
cmake --build build/demo-emscripten --parallel 4 --target frei0r-demo-browser-runtime
ctest --test-dir build/demo-emscripten --output-on-failure -R \
  '^(demo-wasm-adapter-emscripten|demo-wasm-measure-emscripten)$'
```

Then build and check the complete Pages artifact from `docs/`:

```sh
cd docs
export FREI0R_DEMO_RUNTIME_DIR="$PWD/../build/demo-emscripten/examples/browser-demo/runtime"
npm run build
BASE_PATH=/frei0r/ npm run check
```

`npm run build` runs Doxygen, VitePress, and the demo build in that order.
`npm run check` validates maintainer-document links, retained public paths,
demo HTML, the manifest and icons, the service worker, JavaScript, and the
cache-busted Emscripten glue/Wasm pair. The `.wasm` file is a binary artifact;
its `application/wasm` MIME type is an HTTP-server property, which browser
smoke coverage verifies through its localhost server.

For the alternate root-base preview contract, rebuild both Vite applications
and check them with the same runtime directory:

```sh
BASE_PATH=/ npm run build:site
BASE_PATH=/ npm run demo:build
BASE_PATH=/ npm run check
```

## Serve and test locally

For VitePress-only editing, `npm run build:site` is sufficient. To work on the
demo, prepare a current runtime first, then run the Vite development server:

```sh
FREI0R_DEMO_RUNTIME_DIR="$PWD/../build/demo-emscripten/examples/browser-demo/runtime" npm run demo:prepare-runtime
npm run demo:dev
```

To serve the composed artifact after `npm run build`, run `npm run preview` and
visit `http://localhost:4173/frei0r/demo/` (or the URL VitePress prints).

Camera capture is permitted on `localhost`; a remote preview must use HTTPS.
The browser asks for video only after the visitor presses **Start camera**.
Use a current browser with Canvas 2D, WebAssembly, service-worker, and camera
support. A real camera is never required by automated coverage:

```sh
npm run demo:test:service-worker
npm run demo:test:browser
npm run demo:check
```

The browser test uses Chromium with a synthetic RGBA canvas stream. It checks
the complete catalog, deterministic frame digests, parameter changes,
stop/restart/page-hide cleanup, denied and missing-camera states, Wasm HTTP
delivery, worker scope, and cached offline reload. It also checks desktop and
mobile preview dimensions, mouse and synthesized touch swipes, continuous
parameter adjustment, keyboard controls, and the FPS overlay. Set
`DEMO_SMOKE_ARTIFACTS=/path/to/evidence` to retain its JSON summary and desktop
and mobile screenshots. These checks use Chromium, not physical mobile devices.
If a local
browser installation is outside Playwright's default location, set
`DEMO_SMOKE_CHROMIUM_EXECUTABLE=/path/to/chrome`.

## Filters and pixels

The explicit `FREI0R_BROWSER_DEMO_PLUGINS` list in
[`../CMakeLists.txt`](../CMakeLists.txt) is the catalog contract. Keep it to
dependency-free, one-input filters that the adapter supports; do not add
filters needing optional host libraries or dynamic loading. Update the runtime
contract and its tests with every catalog change, then rerun the clean build
and browser smoke commands above.

The catalog contains 88 filters. Optional OpenCV, Cairo, GAVL, and OpenGL
dependencies, dynamic loaders, generators, and mixers remain outside this
single-camera demo. Filters requiring string parameters (`colortap`, `curves`,
`keyspillm0pup`, and `medians`) are also excluded from the current scalar/color/
position controls. `autothresh0ld`, `colorenhance`, and `pixs0r` return failure
from their initialization callback; `3dflippo` fails the repeated-frame WASM
benchmark with an out-of-bounds memory access. Do not re-add these without
fixing and testing their contract.

The host passes Canvas 2D sRGB RGBA bytes directly to `RGBA8888` filters and
also accepts opaque `PACKED32` filters. For `BGRA8888`, it swaps red and blue
in the existing input/output buffers without allocating additional frames.
Frame dimensions are positive multiples of
eight and the adapter owns its aligned input and output storage. The API color
model definitions in [`../include/frei0r.h`](../include/frei0r.h) remain
authoritative.

## Preview controls

On mobile, the preview fills the dynamic viewport and crops the camera image
to cover it. On desktop, the preview occupies most of the viewport. Swipe left
for the next filter and right for the previous filter; browsing wraps around.
Swipe up to increase the dominant parameter and down to decrease it. Movement
locks to one axis after a short threshold, and a vertical drag changes the value
continuously before release. Numeric values stay within the frei0r normalized
range. Boolean parameters switch on/off, color parameters traverse hue, and
position parameters move vertically.

The dominant control comes from filter-specific choices with a metadata-based
fallback in `demo/src/filter-navigation.ts`. The top overlay shows the filter
name and measured rendered FPS; the bottom shows the selected parameter and
value. Filters without controls say so. The preview can be focused to use arrow
keys, and Previous/Next buttons remain available. **Controls** opens the catalog
and full parameter panel while the camera keeps running.

## PWA, privacy, and troubleshooting

The generated worker is scoped to `demo/`. It precaches the revisioned app,
manifest/icons, glue, and Wasm after an online build; revisioned files use
cache-first behavior, while navigation is network-first with the cached shell
as fallback. A new revision precaches before activation and removes older demo
caches. An installed app can launch offline only after it has been cached and
the browser has permission to use a camera.

Processing is local: the app requests `audio: false`, sends no frames to a
server, and stops tracks on **Stop camera**, page hide, and component teardown.

| Symptom | Check |
| --- | --- |
| `FREI0R_DEMO_RUNTIME_DIR` error | Rebuild `frei0r-demo-browser-runtime` and point the variable to its runtime directory. |
| Runtime reported stale | Rebuild after changing files under `examples/browser-demo/runtime/`; the packager rejects stale products. |
| Camera unavailable or denied | Use HTTPS/localhost, grant the browser's camera permission, and confirm an available video device. |
| Worker or offline reload fails | Build online first, reload once for worker control, then retry offline at the exact `/frei0r/demo/` scope. |
| Missing Wasm or incorrect URL | Run `BASE_PATH=/frei0r/ npm run check`; rebuild VitePress before `npm run demo:build`. |

## Deployment verification

Do not deploy an unreviewed branch. The Pages workflow builds the Emscripten
runtime, complete documentation artifact, and browser smoke coverage before it
uploads the Pages artifact. After a permitted deployment, verify:

```text
https://dyne.org/frei0r/
https://dyne.org/frei0r/demo/
https://dyne.org/frei0r/demo/manifest.webmanifest
https://dyne.org/frei0r/codedoc/html/
```

Confirm the demo navigation opens directly, a hard reload returns the demo,
the canonical URL is `https://dyne.org/frei0r/demo/`, the manifest launches
within `/frei0r/demo/`, and the worker does not control documentation routes.
Check that camera permission is explicit, stopping releases the camera, and
GitHub plus <https://t.me/frei0r> remain visible in the app shell. Retained
URLs and the broader production checklist are in [public-urls.md](public-urls.md)
and [DEPLOYMENT.md](DEPLOYMENT.md).
