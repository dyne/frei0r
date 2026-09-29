import { readFile } from 'node:fs/promises';
import { WASI } from 'node:wasi';

const modulePath = process.argv[2];
if (!modulePath) {
  console.error('usage: node run-runtime-smoke-wasi.mjs MODULE.wasm');
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
if (typeof instance.exports.runtime_smoke_run !== 'function') {
  console.error('missing runtime_smoke_run export');
  process.exit(66);
}

const status = instance.exports.runtime_smoke_run();
if (status !== 0) {
  const plugin = Math.trunc(status / 100);
  const stage = status % 100;
  console.error(`runtime smoke failed: plugin=${plugin} stage=${stage} status=${status}`);
  process.exit(status);
}

console.log('runtime smoke passed');
