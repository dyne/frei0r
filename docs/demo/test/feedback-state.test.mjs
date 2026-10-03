import assert from 'node:assert/strict'
import test from 'node:test'

const { composeLiveStatus, connectionMessage, hasRapidVisualChanges, qualityMessage } = await import(process.env.FEEDBACK_STATE_MODULE)

test('keeps material feedback in one polite status sentence', () => {
  assert.equal(composeLiveStatus({
    stageStatus: 'Camera active. Processing frames locally.',
    online: false,
    qualityScale: 0.75,
    rapidVisualChanges: true
  }), 'Camera active. Processing frames locally. You are offline. Cached demo files remain available when already stored on this device. Preview quality reduced to 75% to keep processing responsive. Rapid visual changes are possible with this filter.')
})

test('only adds recovery feedback when it applies', () => {
  assert.equal(connectionMessage(true), undefined)
  assert.equal(qualityMessage(1), undefined)
  assert.equal(qualityMessage(0.5), 'Preview quality reduced to 50% to keep processing responsive.')
  assert.equal(hasRapidVisualChanges('glitch0r'), true)
  assert.equal(hasRapidVisualChanges('vertigo'), false)
})
