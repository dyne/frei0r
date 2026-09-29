import { readFile } from 'node:fs/promises';
import { WASI } from 'node:wasi';

const modulePath = process.argv[2];
if (!modulePath) {
  console.error('usage: node run-wasi-command.mjs MODULE.wasm');
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
wasi.start(await WebAssembly.instantiate(module, imports));
console.log('WASI command passed');
