# Browser runtime measurements

This report is generated from the explicit browser manifest by
`measure-demo-wasm-runtime.mjs`. It measures the Emscripten ES module without
the Svelte host or camera transfer cost. The checked report is updated from a
clean Emscripten build; generated `.mjs` and `.wasm` files are not committed.

## Method

- Scalar Emscripten build with the explicit 18-filter manifest.
- Node calls one initialized module. For each catalog entry and each ABI-valid
  size, a deterministic RGBA input is copied once, then 24 warm updates are
  discarded and 80 `frei0r_demo_update` calls are timed with
  `performance.now()`.
- Report median, p95, and max update duration. These are host-machine
  comparative measurements, not browser-frame-rate claims.
- Raw, gzip level 9, and Brotli quality 11 sizes cover the emitted glue and
  Wasm assets separately and together.

Run from a configured Emscripten build:

```sh
node examples/browser-demo/runtime/measure-demo-wasm-runtime.mjs \
  BUILD/examples/browser-demo/runtime/frei0r-demo-runtime.mjs --json
```

## Recorded evidence

Observed 2026-10-03 with Emscripten 4.0.1 and Node 24.21.0 from a clean
scalar `RelWithDebInfo` build. Timings are milliseconds.

### Final manifest

`brightness`, `bw0r`, `colorize`, `dither`, `distort0r`, `emboss`,
`glitch0r`, `glow`, `heatmap0r`, `hueshift0r`, `invert0r`, `pixeliz0r`,
`posterize`, `rgbsplit0r`, `saturat0r`, `threshold0r`, and `vertigo`.

`colorhalftone` was removed after the same 80-sample run measured 61.89 ms
p95 and 72.38 ms max at 640x480. The final catalog retains color
(`colorize`, `heatmap0r`, `hueshift0r`, `saturat0r`), geometry
(`distort0r`, `pixeliz0r`, `rgbsplit0r`), blur/edge (`glow`, `emboss`),
quantization (`bw0r`, `dither`, `posterize`, `threshold0r`), glitch, and
temporal (`vertigo`) variety.

### Runtime artifact size

| Asset | Raw bytes | gzip-9 bytes | Brotli-11 bytes |
| --- | ---: | ---: | ---: |
| Emscripten glue (`frei0r-demo-runtime.mjs`) | 29,674 | 9,550 | 8,108 |
| Wasm (`frei0r-demo-runtime.wasm`) | 290,733 | 95,854 | 77,536 |
| Total | 320,407 | 105,404 | 85,644 |

### Warm `update` timings

| Filter | 320x240 median / p95 / max | 640x480 median / p95 / max |
| --- | ---: | ---: |
| brightness | 0.0483 / 0.0689 / 0.1502 | 0.1866 / 0.1883 / 0.1943 |
| bw0r | 0.0782 / 0.1247 / 0.1393 | 0.2758 / 0.3035 / 0.3743 |
| colorize | 0.9278 / 1.3050 / 1.5941 | 3.6850 / 3.7162 / 3.7420 |
| dither | 0.1716 / 0.2200 / 0.2383 | 0.6537 / 0.6842 / 0.6864 |
| distort0r | 0.0678 / 0.0959 / 0.0971 | 0.2762 / 0.2899 / 0.2960 |
| emboss | 0.4467 / 0.5712 / 0.5947 | 1.8256 / 1.8673 / 1.8790 |
| glitch0r | 0.0057 / 0.0058 / 0.0078 | 0.0405 / 0.0424 / 0.0435 |
| glow | 0.1797 / 0.1922 / 0.2497 | 0.7590 / 0.7722 / 0.7786 |
| heatmap0r | 0.1471 / 0.1489 / 0.1508 | 0.6032 / 0.6149 / 0.6970 |
| hueshift0r | 0.3809 / 0.3834 / 0.5447 | 1.5626 / 1.5932 / 1.5979 |
| invert0r | 0.0115 / 0.0133 / 0.0189 | 0.0547 / 0.0548 / 0.0572 |
| pixeliz0r | 0.0683 / 0.0849 / 0.2083 | 0.2982 / 0.3011 / 0.3024 |
| posterize | 0.0472 / 0.0484 / 0.0518 | 0.2040 / 0.2059 / 0.2591 |
| rgbsplit0r | 0.1559 / 0.1569 / 0.1590 | 0.6247 / 0.6263 / 0.6266 |
| saturat0r | 0.3053 / 0.3062 / 0.3082 | 1.2209 / 1.3835 / 1.6371 |
| threshold0r | 0.0465 / 0.0465 / 0.0478 | 0.1882 / 0.1898 / 0.1908 |
| vertigo | 0.1051 / 0.1061 / 0.1082 | 0.4239 / 0.4360 / 0.4386 |

`colorize` is the retained 640x480 p95 maximum at 3.72 ms. No
filter-specific quality cap is required by this measurement; camera capture,
Canvas copies, and presentation are intentionally outside this update-only
benchmark and remain host-level measurements.
