import { brotliCompressSync, constants, gzipSync } from 'node:zlib';
import { readFile } from 'node:fs/promises';
import { performance } from 'node:perf_hooks';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

const modulePath = process.argv[2];
const verify = process.argv.includes('--verify');
const json = process.argv.includes('--json');
const warmupFrames = 24;
const measuredFrames = 80;
const sizes = [
  [320, 240],
  [640, 480]
];
const expectedIds = [
  'brightness', 'bw0r', 'colorize', 'dither', 'distort0r',
  'emboss', 'glitch0r', 'glow', 'heatmap0r', 'hueshift0r', 'invert0r',
  'pixeliz0r', 'posterize', 'rgbsplit0r', 'saturat0r', 'threshold0r', 'vertigo'
];

if (!modulePath) {
  console.error('usage: node measure-demo-wasm-runtime.mjs RUNTIME.mjs [--json|--verify]');
  process.exit(64);
}

function percentile(samples, fraction) {
  const ordered = [...samples].sort((left, right) => left - right);
  return ordered[Math.ceil(ordered.length * fraction) - 1];
}

function rounded(value) {
  return Number(value.toFixed(4));
}

function bytesSummary(bytes) {
  return {
    rawBytes: bytes.length,
    gzipBytes: gzipSync(bytes, { level: 9 }).length,
    brotliBytes: brotliCompressSync(bytes, {
      params: { [constants.BROTLI_PARAM_QUALITY]: 11 }
    }).length
  };
}

function deterministicFrame(width, height, seed) {
  const result = new Uint8Array(width * height * 4);
  let state = seed >>> 0;
  for (let index = 0; index < result.length; ++index) {
    state ^= state << 13;
    state ^= state >>> 17;
    state ^= state << 5;
    result[index] = state >>> 24;
  }
  return result;
}

const absoluteModulePath = resolve(modulePath);
const wasmPath = absoluteModulePath.replace(/\.mjs$/, '.wasm');
const { default: createRuntime } = await import(pathToFileURL(absoluteModulePath).href);
const runtime = await createRuntime();
const select = runtime._frei0r_demo_select;
const inputPointer = runtime._frei0r_demo_input_pointer;
const update = runtime._frei0r_demo_update;
const catalogCount = runtime._frei0r_demo_catalog_count;
const catalogId = runtime._frei0r_demo_catalog_id;
const shutdown = runtime._frei0r_demo_shutdown;
const decoder = new TextDecoder();

if ([select, inputPointer, update, catalogCount, catalogId, shutdown].some((entry) =>
  typeof entry !== 'function') || !(runtime.HEAPU8 instanceof Uint8Array)) {
  console.error('runtime does not provide the measurement surface');
  process.exit(65);
}

function text(pointer) {
  let end = pointer;
  while (runtime.HEAPU8[end] !== 0) ++end;
  return decoder.decode(runtime.HEAPU8.subarray(pointer, end));
}

if (catalogCount() !== expectedIds.length || expectedIds.some((id, index) =>
  text(catalogId(index)) !== id)) {
  console.error('measurement catalog differs from the checked manifest');
  process.exit(66);
}

const records = [];
for (const [width, height] of sizes) {
  for (const [catalogIndex, id] of expectedIds.entries()) {
    if (select(catalogIndex, width, height) !== 0) {
      console.error(`selection failed for ${id} at ${width}x${height}`);
      process.exit(67);
    }
    const pointer = inputPointer();
    if (!pointer || pointer % 16) {
      console.error(`input storage contract failed for ${id} at ${width}x${height}`);
      process.exit(68);
    }
    runtime.HEAPU8.set(deterministicFrame(width, height,
      ((catalogIndex + 1) * 0x9e3779b9) ^ width ^ height), pointer);
    for (let frame = 0; frame < warmupFrames; ++frame) {
      if (update(frame / 30) !== 0) {
        console.error(`warmup failed for ${id} at ${width}x${height}`);
        process.exit(69);
      }
    }
    const samples = [];
    for (let frame = 0; frame < measuredFrames; ++frame) {
      const startedAt = performance.now();
      if (update((warmupFrames + frame) / 30) !== 0) {
        console.error(`measurement failed for ${id} at ${width}x${height}`);
        process.exit(70);
      }
      samples.push(performance.now() - startedAt);
    }
    records.push({
      id,
      size: `${width}x${height}`,
      warmupFrames,
      measuredFrames,
      medianMs: rounded(percentile(samples, 0.5)),
      p95Ms: rounded(percentile(samples, 0.95)),
      maxMs: rounded(Math.max(...samples))
    });
  }
}
shutdown();

const artifacts = {
  glue: bytesSummary(await readFile(absoluteModulePath)),
  wasm: bytesSummary(await readFile(wasmPath))
};
artifacts.total = {
  rawBytes: artifacts.glue.rawBytes + artifacts.wasm.rawBytes,
  gzipBytes: artifacts.glue.gzipBytes + artifacts.wasm.gzipBytes,
  brotliBytes: artifacts.glue.brotliBytes + artifacts.wasm.brotliBytes
};
const result = {
  method: `one module, ${warmupFrames} warm frames, ${measuredFrames} timed updates per filter and size`,
  manifest: expectedIds,
  artifacts,
  timings: records
};

if (verify) {
  const validTimings = records.length === expectedIds.length * sizes.length &&
    records.every((record) => record.measuredFrames === measuredFrames &&
      Number.isFinite(record.medianMs) && Number.isFinite(record.p95Ms) &&
      record.medianMs >= 0 && record.p95Ms >= 0);
  const validArtifacts = Object.values(artifacts).every((artifact) =>
    artifact.rawBytes > 0 && artifact.gzipBytes > 0 && artifact.brotliBytes > 0);
  if (!validTimings || !validArtifacts) {
    console.error('measurement evidence is incomplete');
    process.exit(71);
  }
  console.log(`browser runtime measurement passed filters=${expectedIds.length} timings=${records.length}`);
} else if (json) {
  console.log(JSON.stringify(result, null, 2));
} else {
  console.log(JSON.stringify(result));
}
