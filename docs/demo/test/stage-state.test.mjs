import assert from 'node:assert/strict'
import test from 'node:test'

const { resolveStagePresentation, stageFixtureFromSearch } = await import(process.env.STAGE_STATE_MODULE)

const scheduler = { active: false, status: 'Camera processing is stopped.' }
const idleCamera = { status: 'idle' }

test('presents a contextual pre-permission state only after the local runtime is ready', () => {
  const loading = resolveStagePresentation({ camera: idleCamera, runtimeState: 'loading', scheduler, cameraWasStarted: false })
  const ready = resolveStagePresentation({ camera: idleCamera, runtimeState: 'ready', scheduler, cameraWasStarted: false })

  assert.equal(loading.actionDisabled, true)
  assert.equal(loading.actionLabel, 'Preparing camera')
  assert.equal(ready.kind, 'ready')
  assert.equal(ready.actionLabel, 'Start camera')
  assert.match(ready.detail, /browser/)
})

test('distinguishes requesting, running, paused, and stopped camera states', () => {
  const requesting = resolveStagePresentation({ camera: { status: 'starting' }, runtimeState: 'ready', scheduler, cameraWasStarted: false })
  const running = resolveStagePresentation({
    camera: { status: 'active' },
    runtimeState: 'ready',
    scheduler: { active: true, status: 'Processing camera frames locally.' },
    cameraWasStarted: true
  })
  const paused = resolveStagePresentation({ camera: { status: 'active' }, runtimeState: 'ready', scheduler, cameraWasStarted: true })
  const stopped = resolveStagePresentation({ camera: idleCamera, runtimeState: 'ready', scheduler, cameraWasStarted: true })

  assert.equal(requesting.kind, 'requesting')
  assert.equal(requesting.action, 'none')
  assert.equal(running.kind, 'running')
  assert.equal(running.cameraActive, true)
  assert.equal(running.showCanvas, true)
  assert.equal(paused.kind, 'paused')
  assert.equal(stopped.kind, 'stopped')
  assert.equal(stopped.actionLabel, 'Restart camera')
})

test('gives denied, unavailable, and runtime failure states distinct recovery paths', () => {
  const denied = resolveStagePresentation({
    camera: { status: 'error', error: { category: 'permission-denied', message: 'Camera permission was denied.' } },
    runtimeState: 'ready', scheduler, cameraWasStarted: false
  })
  const unavailable = resolveStagePresentation({
    camera: { status: 'error', error: { category: 'no-device', message: 'No usable camera is available.' } },
    runtimeState: 'ready', scheduler, cameraWasStarted: false
  })
  const runtimeFailure = resolveStagePresentation({
    camera: idleCamera, runtimeState: 'failure', runtimeError: 'Unable to load runtime.', scheduler, cameraWasStarted: false
  })

  assert.equal(denied.kind, 'denied')
  assert.match(denied.detail, /settings/)
  assert.equal(unavailable.kind, 'unavailable')
  assert.equal(unavailable.action, 'retry')
  assert.equal(runtimeFailure.kind, 'runtime-failure')
  assert.match(runtimeFailure.detail, /Unable to load runtime/)
})

test('surfaces a processing failure while preserving retry and camera-stop recovery', () => {
  const failure = resolveStagePresentation({
    camera: { status: 'active' },
    runtimeState: 'ready',
    scheduler: { active: false, status: 'Filter failed.', failure: 'Filter failed.' },
    cameraWasStarted: true
  })

  assert.equal(failure.kind, 'processing-failure')
  assert.equal(failure.action, 'retry-processing')
  assert.equal(failure.actionLabel, 'Retry processing')
  assert.equal(failure.cameraActive, true)
  assert.match(failure.detail, /another filter/)
  assert.match(failure.detail, /stop the camera/)
})

test('accepts deterministic development-only stage fixtures', () => {
  assert.equal(stageFixtureFromSearch('?stage=denied', true), 'denied')
  assert.equal(stageFixtureFromSearch('?stage=unknown', true), undefined)
  assert.equal(stageFixtureFromSearch('?stage=running', false), undefined)
})
