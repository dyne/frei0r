import { readFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

const modulePath = process.argv[2];
if (!modulePath) {
  console.error('usage: node run-demo-wasm-runtime.mjs RUNTIME.mjs');
  process.exit(64);
}

const contract = await readFile(new URL('./frei0r-demo-contract.h', import.meta.url), 'utf8');
const expectedIds = [...contract.matchAll(/X\((\w+)\)/g)].map((match) => match[1]);
const requiredExports = [
  'frei0r_demo_catalog_count', 'frei0r_demo_catalog_id',
  'frei0r_demo_catalog_name', 'frei0r_demo_catalog_author',
  'frei0r_demo_catalog_explanation', 'frei0r_demo_catalog_color_model',
  'frei0r_demo_catalog_parameter_count', 'frei0r_demo_select',
  'frei0r_demo_input_pointer', 'frei0r_demo_output_pointer',
  'frei0r_demo_parameter_count', 'frei0r_demo_parameter_type',
  'frei0r_demo_parameter_name', 'frei0r_demo_parameter_explanation',
  'frei0r_demo_get_parameter_scalar', 'frei0r_demo_set_parameter_scalar',
  'frei0r_demo_get_parameter_color_component', 'frei0r_demo_set_parameter_color',
  'frei0r_demo_get_parameter_position_component',
  'frei0r_demo_set_parameter_position', 'frei0r_demo_reset_parameters',
  'frei0r_demo_update', 'frei0r_demo_last_error', 'frei0r_demo_shutdown'
];
const { default: createRuntime } = await import(pathToFileURL(resolve(modulePath)).href);
const runtime = await createRuntime();
const decoder = new TextDecoder();
const call = Object.fromEntries(requiredExports.map((name) => [name, runtime[`_${name}`]]));

if (Object.values(call).some((entry) => typeof entry !== 'function') ||
    !(runtime.HEAPU8 instanceof Uint8Array)) {
  console.error('missing browser adapter export or memory view');
  process.exit(65);
}

function text(pointer) {
  let end = pointer;
  while (runtime.HEAPU8[end] !== 0) ++end;
  return decoder.decode(runtime.HEAPU8.subarray(pointer, end));
}

function frame(width, height, seed) {
  const bytes = new Uint8Array(width * height * 4);
  let state = seed >>> 0;
  for (let index = 0; index < bytes.length; ++index) {
    state ^= state << 13;
    state ^= state >>> 17;
    state ^= state << 5;
    bytes[index] = state >>> 24;
  }
  return bytes;
}

function digest(bytes) {
  let value = 0x811c9dc5;
  for (const byte of bytes)
    value = Math.imul(value ^ byte, 0x01000193) >>> 0;
  return value;
}

for (const [index, id] of expectedIds.entries()) {
  if (text(call.frei0r_demo_catalog_id(index)) !== id ||
      !text(call.frei0r_demo_catalog_name(index)) ||
      !text(call.frei0r_demo_catalog_author(index)) ||
      !text(call.frei0r_demo_catalog_explanation(index))) {
    console.error(`catalog metadata failed at ${id}`);
    process.exit(66);
  }
}
if (call.frei0r_demo_catalog_count() !== expectedIds.length ||
    call.frei0r_demo_select(expectedIds.length, 320, 240) !== 2 ||
    call.frei0r_demo_select(0, 319, 240) !== 3) {
  console.error('catalog or bounds contract failed');
  process.exit(67);
}

for (const [catalogIndex, id] of expectedIds.entries()) {
  if (call.frei0r_demo_select(catalogIndex, 320, 240) !== 0) {
    console.error(`select failed for ${id}`);
    process.exit(68);
  }
  const inputPointer = call.frei0r_demo_input_pointer();
  const outputPointer = call.frei0r_demo_output_pointer();
  if (!inputPointer || !outputPointer || inputPointer % 16 || outputPointer % 16) {
    console.error(`unaligned frame storage for ${id}`);
    process.exit(69);
  }
  runtime.HEAPU8.set(frame(320, 240, catalogIndex + 1), inputPointer);
  for (let parameterIndex = 0;
       parameterIndex < call.frei0r_demo_parameter_count(); ++parameterIndex) {
    const type = call.frei0r_demo_parameter_type(parameterIndex);
    // Several portable filters legitimately provide an empty explanation.
    if (!text(call.frei0r_demo_parameter_name(parameterIndex))) {
      console.error(`parameter metadata failed for ${id}`);
      process.exit(70);
    }
    if (type === 0 || type === 1) {
      const value = call.frei0r_demo_get_parameter_scalar(parameterIndex);
      if (!Number.isFinite(value) ||
          call.frei0r_demo_set_parameter_scalar(parameterIndex, value < 0.5 ? 0.75 : 0.25) !== 0) {
        console.error(`scalar parameter failed for ${id}`);
        process.exit(71);
      }
    } else if (type === 2) {
      if (call.frei0r_demo_set_parameter_color(parameterIndex, 0.25, 0.5, 0.75) !== 0 ||
          Math.abs(call.frei0r_demo_get_parameter_color_component(parameterIndex, 2) - 0.75) > 0.0001) {
        console.error(`color parameter failed for ${id}`);
        process.exit(72);
      }
    } else if (type === 3) {
      if (call.frei0r_demo_set_parameter_position(parameterIndex, 0.25, 0.75) !== 0 ||
          Math.abs(call.frei0r_demo_get_parameter_position_component(parameterIndex, 1) - 0.75) > 0.000001) {
        console.error(`position parameter failed for ${id}`);
        process.exit(73);
      }
    } else {
      console.error(`unsupported parameter type for ${id}`);
      process.exit(74);
    }
  }
  if (call.frei0r_demo_update(catalogIndex / 30) !== 0 ||
      digest(runtime.HEAPU8.subarray(outputPointer, outputPointer + 320 * 240 * 4)) === 0 ||
      call.frei0r_demo_reset_parameters() !== 0) {
    console.error(`frame update failed for ${id}`);
    process.exit(75);
  }
}

const wasmExports = WebAssembly.Module.exports(new WebAssembly.Module(
  await readFile(resolve(modulePath).replace(/\.mjs$/, '.wasm'))
));
const exportedAdapterCalls = wasmExports.filter((entry) =>
  entry.name.startsWith('frei0r_demo_')).map((entry) => entry.name).sort();
if (!wasmExports.some((entry) => entry.kind === 'memory') ||
    JSON.stringify(exportedAdapterCalls) !== JSON.stringify([...requiredExports].sort()) ||
    wasmExports.some((entry) => entry.name.includes('f0r_bundle') ||
      entry.name.includes('descriptor'))) {
  console.error('runtime does not expose the bounded adapter and memory surface');
  process.exit(76);
}
call.frei0r_demo_shutdown();
if (call.frei0r_demo_select(0, 320, 240) !== 10) {
  console.error('shutdown contract failed');
  process.exit(77);
}
console.log(`browser adapter contract passed filters=${expectedIds.length}`);
