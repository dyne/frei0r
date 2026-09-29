import { createRequire } from 'node:module';
import { resolve } from 'node:path';

const [scalarPath, simdPath] = process.argv.slice(2);
if (!scalarPath || !simdPath) {
  console.error('usage: node compare-bundle-emscripten-profiles.mjs SCALAR.js SIMD.js');
  process.exit(64);
}

const require = createRequire(import.meta.url);
const decoder = new TextDecoder();
// These descriptors explicitly seed private generators from time(NULL).  They
// remain covered by every registry run, but their pixels cannot be compared
// between separately-created scalar and SIMD module instances.
const crossProfilePixelExclusions = new Set(['delaygrab', 'partik0l']);

function cString(module, pointer) {
  let end = pointer;
  while (module.HEAPU8[end] !== 0) ++end;
  return decoder.decode(module.HEAPU8.subarray(pointer, end));
}

async function load(path) {
  const createBundleRegistry = require(resolve(path));
  const module = await createBundleRegistry();
  const api = {
    run: module._f0r_bundle_emscripten_run,
    count: module._f0r_bundle_emscripten_output_count,
    digestAt: module._f0r_bundle_emscripten_output_digest_at,
    idAt: module._f0r_bundle_emscripten_output_id_at,
  };
  if (Object.values(api).some((value) => typeof value !== 'function'))
    throw new Error(`missing registry comparison exports in ${path}`);
  return { module, api };
}

function run(label, loaded) {
  if (loaded.api.run() !== 0)
    throw new Error(`${label} registry execution failed`);
  const count = loaded.api.count();
  return Array.from({ length: count }, (_, index) => ({
    id: cString(loaded.module, loaded.api.idAt(index)),
    digest: loaded.api.digestAt(index) >>> 0,
  }));
}

const scalar = await load(scalarPath);
const simd = await load(simdPath);
const scalarFirst = run('scalar first', scalar);
const scalarSecond = run('scalar second', scalar);
const simdFirst = run('SIMD first', simd);
const simdSecond = run('SIMD second', simd);

if (scalarFirst.length !== simdFirst.length || scalarFirst.length !== scalarSecond.length ||
    scalarFirst.length !== simdSecond.length)
  throw new Error('profile descriptor counts differ');

const excluded = new Set();
for (let index = 0; index < scalarFirst.length; ++index) {
  const scalarEntry = scalarFirst[index];
  const simdEntry = simdFirst[index];
  if (scalarEntry.id !== simdEntry.id)
    throw new Error(`profile descriptor ID mismatch at index=${index}: ${scalarEntry.id} != ${simdEntry.id}`);
  if (crossProfilePixelExclusions.has(scalarEntry.id)) {
    excluded.add(scalarEntry.id);
    continue;
  }
  if (scalarEntry.digest !== scalarSecond[index].digest ||
      simdEntry.digest !== simdSecond[index].digest)
    throw new Error(`unexpected non-deterministic output: id=${scalarEntry.id}`);
  if (scalarEntry.digest !== simdEntry.digest)
    throw new Error(`SIMD output mismatch: id=${scalarEntry.id} scalar=${scalarEntry.digest} simd=${simdEntry.digest}`);
}

if (excluded.size !== crossProfilePixelExclusions.size)
  throw new Error('a reviewed cross-profile exclusion is missing from the registry');
console.log(`bundle profile parity passed descriptors=${scalarFirst.length} cross-profile-excluded=${excluded.size} ids=${[...excluded].join(',')}`);
