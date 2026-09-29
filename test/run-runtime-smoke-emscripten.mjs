import { createRequire } from 'node:module';
import { resolve } from 'node:path';

const modulePath = process.argv[2];
if (!modulePath) {
  console.error('usage: node run-runtime-smoke-emscripten.mjs MODULE.js');
  process.exit(64);
}

const require = createRequire(import.meta.url);
const createRuntimeSmoke = require(resolve(modulePath));
const module = await createRuntimeSmoke();

if (typeof module._runtime_smoke_run !== 'function') {
  console.error('missing _runtime_smoke_run export');
  process.exit(65);
}

const status = module._runtime_smoke_run();
if (status !== 0) {
  const plugin = Math.trunc(status / 100);
  const stage = status % 100;
  console.error(`runtime smoke failed: plugin=${plugin} stage=${stage} status=${status}`);
  process.exit(status);
}

console.log('runtime smoke passed');
