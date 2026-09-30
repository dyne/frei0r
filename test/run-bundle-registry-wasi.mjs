import { readFile } from 'node:fs/promises';
import { WASI } from 'node:wasi';

const modulePath = process.argv[2];
if (!modulePath) {
  console.error('usage: node run-bundle-registry-wasi.mjs MODULE.wasm');
  process.exit(64);
}

const module = await WebAssembly.compile(await readFile(modulePath));
const wasi = new WASI({ version: 'preview1' });
const imports = {};
for (const entry of WebAssembly.Module.imports(module)) {
  if (entry.module !== 'wasi_snapshot_preview1' || entry.kind !== 'function' ||
      typeof wasi.wasiImport[entry.name] !== 'function') {
    console.error(`unsupported import: ${entry.module}.${entry.name} (${entry.kind})`);
    process.exit(65);
  }
  imports[entry.module] ??= {};
  imports[entry.module][entry.name] = wasi.wasiImport[entry.name];
}

const instance = await WebAssembly.instantiate(module, imports);
const { memory, f0r_bundle_wasi_run: run, f0r_bundle_wasi_failure_id: failureId,
  f0r_bundle_wasi_failure_stage: failureStage,
  f0r_bundle_wasi_output_digest: outputDigest,
  f0r_bundle_wasi_application_frame_count: applicationFrameCount,
  f0r_bundle_wasi_parameter_change_count: parameterChangeCount } = instance.exports;
if (!(memory instanceof WebAssembly.Memory) || typeof run !== 'function' ||
    typeof outputDigest !== 'function' || typeof failureId !== 'function' ||
    typeof failureStage !== 'function' || typeof applicationFrameCount !== 'function' ||
    typeof parameterChangeCount !== 'function') {
  console.error('missing bounded bundle registry runner exports');
  process.exit(66);
}
wasi.initialize(instance);

function stringAt(pointer) {
  const bytes = new Uint8Array(memory.buffer);
  let end = pointer;
  while (end < bytes.length && bytes[end] !== 0) end += 1;
  return new TextDecoder().decode(bytes.subarray(pointer, end));
}

const status = run();
if (status !== 0) {
  console.error(`bundle registry failed: plugin=${stringAt(failureId())} stage=${stringAt(failureStage())} status=${status}`);
  process.exit(status);
}
const frames = applicationFrameCount() >>> 0;
const parameterChanges = parameterChangeCount() >>> 0;
if (frames < 2 || parameterChanges < 1) {
  console.error(`application coverage missing: frames=${frames} parameter_changes=${parameterChanges}`);
  process.exit(67);
}
console.log(`bundle application contract passed digest=${outputDigest() >>> 0} frames=${frames} parameter_changes=${parameterChanges}`);
