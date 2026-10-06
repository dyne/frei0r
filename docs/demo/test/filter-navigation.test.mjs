import assert from 'node:assert/strict'
import test from 'node:test'

const { navigateFilterKey, dominantParameter, gestureAxis, swipeParameterValue, parameterLabel } = await import(process.env.FILTER_NAVIGATION_MODULE)

test('swipes lock only after a deliberate movement on a dominant axis', () => {
  assert.equal(gestureAxis(5, 4), undefined)
  assert.equal(gestureAxis(20, 20), undefined)
  assert.equal(gestureAxis(-60, 12), 'horizontal')
  assert.equal(gestureAxis(10, -80), 'vertical')
})

test('dominant control prefers expressive parameters and explicit filter choices', () => {
  const controls = [
    { index: 0, name: 'Enabled', kind: 'boolean', value: true },
    { index: 1, name: 'Frequency', kind: 'number', value: 0.2 },
    { index: 2, name: 'Amplitude', kind: 'number', value: 0.4 }
  ]
  assert.equal(dominantParameter('distort0r', controls).index, 2)
  assert.equal(dominantParameter('unknown', controls).index, 2)
  assert.equal(dominantParameter('bw0r', []), undefined)
})

test('vertical adjustments are continuous, bounded, and support every exposed parameter kind', () => {
  const number = { kind: 'number', value: 0.4 }
  assert.equal(swipeParameterValue(number, 0.125), 0.525)
  assert.equal(swipeParameterValue(number, 2), 1)
  assert.equal(swipeParameterValue(number, -2), 0)
  assert.equal(swipeParameterValue({ kind: 'boolean', value: false }, 0.2), true)
  assert.equal(swipeParameterValue({ kind: 'boolean', value: true }, -0.2), false)
  assert.deepEqual(swipeParameterValue({ kind: 'position', value: [0.3, 0.4] }, 0.2), [0.3, 0.6000000000000001])
  const color = swipeParameterValue({ kind: 'color', value: [1, 0, 0] }, 1 / 3)
  assert.ok(color[1] > 0.99 && color[0] < 0.01 && color[2] < 0.01)
  assert.equal(parameterLabel({ kind: 'number', value: 0.125 }), '0.125')
  assert.equal(parameterLabel({ kind: 'color', value: [1, 0, 0] }), '#ff0000')
})

function selectionHarness(initialIndex = 4) {
  let activeIndex = initialIndex
  const schedulerSelections = []
  return {
    select(index) {
      activeIndex = index
      schedulerSelections.push(index)
    },
    get activeIndex() {
      return activeIndex
    },
    schedulerSelections
  }
}

test('Tab and unrelated keys preserve the active and scheduled filter', () => {
  const harness = selectionHarness()
  const catalog = [2, 4, 7]

  assert.equal(navigateFilterKey('Tab', 7, catalog, harness.select), false)
  assert.equal(navigateFilterKey('Enter', 7, catalog, harness.select), false)
  assert.equal(harness.activeIndex, 4)
  assert.deepEqual(harness.schedulerSelections, [])
})

test('arrow and boundary keys select exactly once from the focused filter', () => {
  const catalog = [2, 4, 7]
  for (const [key, focusedIndex, expectedIndex] of [
    ['ArrowLeft', 4, 2],
    ['ArrowRight', 4, 7],
    ['Home', 7, 2],
    ['End', 2, 7]
  ]) {
    const harness = selectionHarness()
    assert.equal(navigateFilterKey(key, focusedIndex, catalog, harness.select), true)
    assert.equal(harness.activeIndex, expectedIndex)
    assert.deepEqual(harness.schedulerSelections, [expectedIndex])
  }
})
