import assert from 'node:assert/strict'
import test from 'node:test'

const { navigateFilterKey } = await import(process.env.FILTER_NAVIGATION_MODULE)

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
