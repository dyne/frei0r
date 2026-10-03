import assert from 'node:assert/strict'
import test from 'node:test'

const { CameraService } = await import(process.env.CAMERA_MODULE)

class FakeTrack {
  listeners = new Set()
  stops = 0

  stop() {
    this.stops += 1
  }

  addEventListener(_type, listener) {
    this.listeners.add(listener)
  }

  removeEventListener(_type, listener) {
    this.listeners.delete(listener)
  }

  end() {
    for (const listener of [...this.listeners]) listener()
  }
}

function stream(...tracks) {
  return { getTracks: () => tracks }
}

function deferred() {
  let resolve
  let reject
  const promise = new Promise((resolvePromise, rejectPromise) => {
    resolve = resolvePromise
    reject = rejectPromise
  })
  return { promise, resolve, reject }
}

function platform(getUserMedia) {
  let pageHideListener
  return {
    getUserMedia,
    addPageHideListener(listener) { pageHideListener = listener },
    removePageHideListener(listener) { if (pageHideListener === listener) pageHideListener = undefined },
    pageHide() { pageHideListener?.() }
  }
}

function failureCategory(result) {
  assert.equal(result.ok, false)
  if (result.ok) throw new Error('Expected camera startup to fail.')
  return result.error.category
}

test('starts a video-only stream and stops it explicitly', async () => {
  const track = new FakeTrack()
  let constraints
  const service = new CameraService(platform(async (requested) => {
    constraints = requested
    return stream(track)
  }))

  const result = await service.start()
  assert.equal(result.ok, true)
  assert.deepEqual(constraints, { audio: false, video: true })
  assert.equal(service.snapshot.status, 'active')

  service.stop()
  service.stop()
  assert.equal(track.stops, 1)
  assert.equal(service.snapshot.status, 'idle')
})

test('categorizes denied, unsupported, and absent-device camera requests', async () => {
  const denied = new CameraService(platform(async () => {
    throw { name: 'NotAllowedError' }
  }))
  const absentDevice = new CameraService(platform(async () => {
    throw { name: 'NotFoundError' }
  }))
  const unsupported = new CameraService(platform(undefined))

  assert.equal(failureCategory(await denied.start()), 'permission-denied')
  assert.equal(failureCategory(await absentDevice.start()), 'no-device')
  assert.equal(failureCategory(await unsupported.start()), 'unsupported')
})

test('cancels an in-flight request without retaining its tracks', async () => {
  const pending = deferred()
  const track = new FakeTrack()
  const service = new CameraService(platform(async () => pending.promise))

  const starting = service.start()
  service.stop()
  pending.resolve(stream(track))

  assert.equal(failureCategory(await starting), 'cancelled')
  assert.equal(track.stops, 1)
  assert.equal(service.snapshot.status, 'idle')
})

test('restarts cleanly and handles device loss and pagehide', async () => {
  const first = new FakeTrack()
  const second = new FakeTrack()
  const third = new FakeTrack()
  const streams = [stream(first), stream(second), stream(third)]
  const fakePlatform = platform(async () => streams.shift())
  const service = new CameraService(fakePlatform)

  await service.start()
  await service.start()
  assert.equal(first.stops, 1)
  second.end()
  assert.equal(second.stops, 1)
  assert.equal(service.snapshot.error?.category, 'device-lost')

  await service.start()
  fakePlatform.pageHide()
  assert.equal(third.stops, 1)
  assert.equal(service.snapshot.status, 'idle')
})

test('destroy removes pagehide cleanup after stopping the active stream', async () => {
  const track = new FakeTrack()
  const fakePlatform = platform(async () => stream(track))
  const service = new CameraService(fakePlatform)
  await service.start()

  service.destroy()
  fakePlatform.pageHide()
  assert.equal(track.stops, 1)
})
