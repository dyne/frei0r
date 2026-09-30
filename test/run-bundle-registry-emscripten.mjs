import { createRequire } from 'node:module';
import { resolve } from 'node:path';

const modulePath = process.argv[2];
if (!modulePath) {
  console.error('usage: node run-bundle-registry-emscripten.mjs MODULE.js');
  process.exit(64);
}

const require = createRequire(import.meta.url);
const createBundleRegistry = require(resolve(modulePath));
const module = await createBundleRegistry();
const decoder = new TextDecoder();
function cString(pointer) {
  let end = pointer;
  while (module.HEAPU8[end] !== 0) ++end;
  return decoder.decode(module.HEAPU8.subarray(pointer, end));
}
const run = module._f0r_bundle_emscripten_run;
const outputDigest = module._f0r_bundle_emscripten_output_digest;
const applicationFrameCount = module._f0r_bundle_emscripten_application_frame_count;
const parameterChangeCount = module._f0r_bundle_emscripten_parameter_change_count;
const failureId = module._f0r_bundle_emscripten_failure_id;
const failureStage = module._f0r_bundle_emscripten_failure_stage;
const outputCount = module._f0r_bundle_emscripten_output_count;
const outputDigestAt = module._f0r_bundle_emscripten_output_digest_at;
const outputIdAt = module._f0r_bundle_emscripten_output_id_at;
if (typeof run !== 'function' || typeof outputDigest !== 'function' ||
    typeof failureId !== 'function' ||
    typeof failureStage !== 'function' || typeof outputCount !== 'function' ||
    typeof outputDigestAt !== 'function' || typeof outputIdAt !== 'function' ||
    typeof applicationFrameCount !== 'function' || typeof parameterChangeCount !== 'function') {
  console.error('missing bounded bundle registry runner exports');
  process.exit(65);
}

const status = run();
if (status !== 0) {
  console.error(`bundle registry failed: plugin=${cString(failureId())} stage=${cString(failureStage())} status=${status}`);
  process.exit(status);
}
const frames = applicationFrameCount() >>> 0;
const parameterChanges = parameterChangeCount() >>> 0;
if (frames < 2 || parameterChanges < 1) {
  console.error(`application coverage missing: frames=${frames} parameter_changes=${parameterChanges}`);
  process.exit(66);
}
console.log(`bundle application contract passed digest=${outputDigest() >>> 0} frames=${frames} parameter_changes=${parameterChanges}`);
if (process.argv.includes('--trace')) {
  for (let index = 0; index < outputCount(); ++index)
    console.log(`bundle registry trace index=${index} id=${cString(outputIdAt(index))} digest=${outputDigestAt(index) >>> 0}`);
}
