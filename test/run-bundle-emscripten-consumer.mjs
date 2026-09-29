import { createRequire } from 'node:module';
import { resolve } from 'node:path';

const modulePath = process.argv[2];
if (!modulePath) {
  console.error('usage: node run-bundle-emscripten-consumer.mjs MODULE.js');
  process.exit(64);
}

const require = createRequire(import.meta.url);
const createBundleConsumer = require(resolve(modulePath));
const module = await createBundleConsumer();
if (typeof module._f0r_bundle_emscripten_consumer_run !== 'function') {
  console.error('missing public archive consumer export');
  process.exit(65);
}
const status = module._f0r_bundle_emscripten_consumer_run();
if (status !== 0) {
  console.error(`public archive consumer failed: status=${status}`);
  process.exit(status);
}
console.log('public archive consumer passed');
