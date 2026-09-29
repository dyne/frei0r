import { readFile } from 'node:fs/promises';
import { WASI } from 'node:wasi';

const [modulePath, functionName] = process.argv.slice(2);
if (!modulePath || !functionName) process.exit(64);
const module = await WebAssembly.compile(await readFile(modulePath));
const wasi = new WASI({ version: 'preview1' });
const imports = {};
for (const entry of WebAssembly.Module.imports(module)) {
  if (entry.module !== 'wasi_snapshot_preview1' || entry.kind !== 'function' ||
      typeof wasi.wasiImport[entry.name] !== 'function') process.exit(65);
  imports[entry.module] ??= {};
  imports[entry.module][entry.name] = wasi.wasiImport[entry.name];
}
const instance = await WebAssembly.instantiate(module, imports);
if (typeof instance.exports[functionName] !== 'function') process.exit(66);
wasi.initialize(instance);
process.exit(instance.exports[functionName]());
