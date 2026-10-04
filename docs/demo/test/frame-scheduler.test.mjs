import assert from 'node:assert/strict'
import test from 'node:test'

const { FrameScheduler, deriveFrameDimensions } = await import(process.env.FRAME_SCHEDULER_MODULE)

class FakeVideo {
  constructor(width = 640, height = 480, withVideoCallback = true) {
    this.videoWidth = width
    this.videoHeight = height
    this.withVideoCallback = withVideoCallback
    this.callbacks = new Map()
    this.nextHandle = 1
  }

  requestVideoFrameCallback(callback) {
    if (!this.withVideoCallback) return undefined
    const handle = this.nextHandle++
    this.callbacks.set(handle, callback)
    return handle
  }

  cancelVideoFrameCallback(handle) {
    this.callbacks.delete(handle)
  }

  frame(time) {
    const [handle, callback] = this.callbacks.entries().next().value ?? []
    if (!callback) throw new Error('No video callback was scheduled.')
    this.callbacks.delete(handle)
    callback(time, {})
  }
}

class FakePlatform {
  constructor() {
    this.callbacks = new Map()
    this.nextHandle = 1
    this.time = 0
  }

  requestAnimationFrame(callback) {
    const handle = this.nextHandle++
    this.callbacks.set(handle, callback)
    return handle
  }

  cancelAnimationFrame(handle) {
    this.callbacks.delete(handle)
  }

  now() {
    return this.time
  }

  frame(time) {
    const [handle, callback] = this.callbacks.entries().next().value ?? []
    if (!callback) throw new Error('No animation callback was scheduled.')
    this.callbacks.delete(handle)
    callback(time)
  }
}

function renderer() {
  return {
    configurations: [],
    frames: [],
    configure(filter, dimensions) {
      this.configurations.push({ filter, dimensions })
    },
    render(_video, time) {
      this.frames.push(time)
    }
  }
}

test('derives bounded dimensions in multiples of eight', () => {
  const dimensions = deriveFrameDimensions(1920, 1080)
  assert.deepEqual(dimensions, { width: 736, height: 408 })
  assert.ok(dimensions.width * dimensions.height <= 640 * 480)
  assert.equal(dimensions.width % 8, 0)
  assert.equal(dimensions.height % 8, 0)
})

test('converts browser millisecond timestamps to frei0r ABI seconds and stops cleanly', () => {
  const video = new FakeVideo()
  const pipeline = renderer()
  const platform = new FakePlatform()
  const scheduler = new FrameScheduler(pipeline, video, platform)
  scheduler.selectFilter(0)
  scheduler.start()
  assert.equal(video.callbacks.size, 1)

  // include/frei0r.h defines f0r_update time in seconds; browser callbacks use milliseconds.
  video.frame(10)
  video.frame(20)
  video.frame(30)
  assert.deepEqual(pipeline.frames, [0.01, 0.02, 0.03])
  assert.equal(pipeline.configurations.length, 1)
  assert.equal(scheduler.snapshot.renderedFrames, 3)
  assert.equal(video.callbacks.size, 1)

  scheduler.stop()
  assert.equal(video.callbacks.size, 0)
})

test('coalesces rapid filter changes and rebuilds after an orientation change', () => {
  const video = new FakeVideo()
  const pipeline = renderer()
  const scheduler = new FrameScheduler(pipeline, video, new FakePlatform())
  scheduler.selectFilter(0)
  scheduler.start()
  video.frame(1)

  scheduler.selectFilter(1)
  scheduler.selectFilter(2)
  scheduler.selectFilter(1)
  video.videoWidth = 480
  video.videoHeight = 640
  video.frame(2)

  assert.deepEqual(pipeline.configurations.map(({ filter }) => filter), [0, 1])
  assert.notDeepEqual(pipeline.configurations[0].dimensions, pipeline.configurations[1].dimensions)
  assert.equal(scheduler.snapshot.renderedFrames, 2)
})

test('publishes the pending and applied filter so controls can reflect serialized selection', () => {
  const video = new FakeVideo()
  const pipeline = renderer()
  const scheduler = new FrameScheduler(pipeline, video, new FakePlatform())
  scheduler.selectFilter(3)
  assert.equal(scheduler.snapshot.pendingFilter, 3)
  assert.equal(scheduler.snapshot.selectedFilter, undefined)
  scheduler.start()
  video.frame(1)
  assert.equal(scheduler.snapshot.pendingFilter, undefined)
  assert.equal(scheduler.snapshot.selectedFilter, 3)
})

test('falls back to animation frames when video callbacks are unavailable', () => {
  const video = new FakeVideo(640, 480, false)
  video.requestVideoFrameCallback = undefined
  const pipeline = renderer()
  const platform = new FakePlatform()
  const scheduler = new FrameScheduler(pipeline, video, platform)
  scheduler.selectFilter(0)
  scheduler.start()
  assert.equal(platform.callbacks.size, 1)
  platform.frame(5)
  assert.deepEqual(pipeline.frames, [0.005])
  assert.equal(platform.callbacks.size, 1)
})

test('drops reentrant frames and reduces preview quality after a long update', () => {
  const video = new FakeVideo()
  const platform = new FakePlatform()
  const pipeline = renderer()
  let scheduler
  pipeline.render = (_video, time) => {
    scheduler.receiveFrame(time + 0.1)
    platform.time = 100
    pipeline.frames.push(time)
  }
  scheduler = new FrameScheduler(pipeline, video, platform, 50)
  scheduler.selectFilter(0)
  scheduler.start()
  video.frame(1)

  assert.equal(scheduler.snapshot.renderedFrames, 1)
  assert.equal(scheduler.snapshot.droppedFrames, 1)
  assert.ok(scheduler.snapshot.qualityScale < 1)
  assert.match(scheduler.snapshot.status, /Reducing preview quality/)
})

test('stops retrying after a processing failure and can recover explicitly', () => {
  const video = new FakeVideo()
  const pipeline = renderer()
  const scheduler = new FrameScheduler(pipeline, video, new FakePlatform())
  let attempts = 0
  pipeline.render = (_video, time) => {
    ++attempts
    if (attempts === 1) throw new Error('The selected filter failed.')
    pipeline.frames.push(time)
  }
  scheduler.selectFilter(0)
  scheduler.start()
  video.frame(1000)

  assert.equal(scheduler.snapshot.active, false)
  assert.equal(scheduler.snapshot.failure, 'The selected filter failed.')
  assert.equal(video.callbacks.size, 0)
  assert.equal(attempts, 1)

  scheduler.start()
  video.frame(2000)
  assert.equal(scheduler.snapshot.failure, undefined)
  assert.equal(scheduler.snapshot.active, true)
  assert.deepEqual(pipeline.frames, [2])
  assert.equal(video.callbacks.size, 1)
})
