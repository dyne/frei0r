import assert from 'node:assert/strict'
import test from 'node:test'

const { readFilterCatalog } = await import(process.env.FILTER_CATALOG_MODULE)

function write(heap, offset, value) {
  heap.set(new TextEncoder().encode(`${value}\0`), offset)
  return offset
}

test('reads runtime-backed filter names, explanations, and parameter counts', () => {
  const heap = new Uint8Array(256)
  const runtime = {
    HEAPU8: heap,
    _frei0r_demo_catalog_count: () => 2,
    _frei0r_demo_catalog_id: (index) => write(heap, index ? 64 : 8, index ? 'glow' : 'contrast'),
    _frei0r_demo_catalog_name: (index) => write(heap, index ? 112 : 32, index ? 'Glow' : 'Contrast'),
    _frei0r_demo_catalog_explanation: (index) => write(heap, index ? 160 : 80, index ? 'Adds a glow.' : 'Shapes contrast.'),
    _frei0r_demo_catalog_parameter_count: (index) => index + 1
  }
  assert.deepEqual(readFilterCatalog(runtime), [
    { index: 0, id: 'contrast', name: 'Contrast', explanation: 'Shapes contrast.', parameterCount: 1 },
    { index: 1, id: 'glow', name: 'Glow', explanation: 'Adds a glow.', parameterCount: 2 }
  ])
})

test('rejects a missing catalog and malformed runtime text pointers', () => {
  const runtime = {
    HEAPU8: new Uint8Array(8),
    _frei0r_demo_catalog_count: () => 0,
    _frei0r_demo_catalog_id: () => 0,
    _frei0r_demo_catalog_name: () => 0,
    _frei0r_demo_catalog_explanation: () => 0,
    _frei0r_demo_catalog_parameter_count: () => 0
  }
  assert.throws(() => readFilterCatalog(runtime), /usable filter catalog/)
  assert.throws(() => readFilterCatalog({
    ...runtime,
    _frei0r_demo_catalog_count: () => 1,
    _frei0r_demo_catalog_id: () => 9
  }), /invalid filter identifier metadata/)
})
