import assert from 'node:assert/strict'
import test from 'node:test'

const { FilterParameters } = await import(process.env.FILTER_PARAMETERS_MODULE)

function writeText(heap, offset, value) {
  heap.set(new TextEncoder().encode(`${value}\0`), offset)
  return offset
}

function createRuntime() {
  const heap = new Uint8Array(512)
  const entries = [
    { type: 0, name: writeText(heap, 8, 'Enabled'), explanation: writeText(heap, 32, 'Toggle effect'), value: 1, default: 1 },
    { type: 1, name: writeText(heap, 64, 'Amount'), explanation: writeText(heap, 88, 'Effect amount'), value: 0.25, default: 0.25 },
    { type: 2, name: writeText(heap, 120, 'Tint'), explanation: writeText(heap, 144, 'Effect color'), value: [0.1, 0.2, 0.3], default: [0.1, 0.2, 0.3] },
    { type: 3, name: writeText(heap, 176, 'Center'), explanation: writeText(heap, 200, 'Effect center'), value: [0.4, 0.5], default: [0.4, 0.5] },
    { type: 4, name: writeText(heap, 232, 'Hidden'), explanation: writeText(heap, 256, 'String value'), value: 'excluded', default: 'excluded' }
  ]
  const runtime = {
    HEAPU8: heap,
    selectCalls: 0,
    updateCalls: 0,
    _frei0r_demo_parameter_count: () => entries.length,
    _frei0r_demo_parameter_type: (index) => entries[index].type,
    _frei0r_demo_parameter_name: (index) => entries[index].name,
    _frei0r_demo_parameter_explanation: (index) => entries[index].explanation,
    _frei0r_demo_get_parameter_scalar: (index) => entries[index].value,
    _frei0r_demo_set_parameter_scalar: (index, value) => {
      entries[index].value = value
      return 0
    },
    _frei0r_demo_get_parameter_color_component: (index, component) => entries[index].value[component],
    _frei0r_demo_set_parameter_color: (index, red, green, blue) => {
      entries[index].value = [red, green, blue]
      return 0
    },
    _frei0r_demo_get_parameter_position_component: (index, component) => entries[index].value[component],
    _frei0r_demo_set_parameter_position: (index, x, y) => {
      entries[index].value = [x, y]
      return 0
    },
    _frei0r_demo_reset_parameters: () => {
      for (const entry of entries) entry.value = Array.isArray(entry.default) ? [...entry.default] : entry.default
      return 0
    },
    _frei0r_demo_last_error: () => 0
  }
  return { runtime, entries }
}

test('derives only supported controls and round-trips each typed value through Wasm', () => {
  const { runtime } = createRuntime()
  const parameters = new FilterParameters(runtime)
  assert.deepEqual(parameters.refresh().map(({ kind, name }) => [kind, name]), [
    ['boolean', 'Enabled'],
    ['number', 'Amount'],
    ['color', 'Tint'],
    ['position', 'Center']
  ])

  parameters.set(0, false)
  parameters.set(1, 0.75)
  parameters.set(2, [0.9, 0.8, 0.7])
  parameters.set(3, [0.6, 0.5])
  assert.deepEqual(parameters.values.map(({ value }) => value), [false, 0.75, [0.9, 0.8, 0.7], [0.6, 0.5]])
  assert.equal(runtime.selectCalls, 0)
})

test('applies rapid changes during rendering without selecting a new module', () => {
  const { runtime, entries } = createRuntime()
  const parameters = new FilterParameters(runtime)
  parameters.refresh()

  for (const value of [0.1, 0.2, 0.3, 0.4]) {
    parameters.set(1, value)
    runtime.updateCalls += 1
    assert.equal(entries[1].value, value)
  }
  assert.equal(runtime.updateCalls, 4)
  assert.equal(runtime.selectCalls, 0)
})

test('reset refreshes defaults after the runtime reconstructs its active instance', () => {
  const { runtime } = createRuntime()
  const parameters = new FilterParameters(runtime)
  parameters.refresh()
  parameters.set(1, 0.9)
  parameters.set(2, [0.7, 0.6, 0.5])
  parameters.reset()

  assert.deepEqual(parameters.values.map(({ value }) => value), [true, 0.25, [0.1, 0.2, 0.3], [0.4, 0.5]])
})
