import assert from 'node:assert/strict'
import { createHash } from 'node:crypto'
import test from 'node:test'

const { ColorModel, FramePipeline } = await import(process.env.FRAME_PIPELINE_MODULE)

class FakeContext {
  constructor(bytes) {
    this.bytes = bytes
    this.draws = 0
    this.presented = []
  }

  drawImage() {
    this.draws += 1
  }

  getImageData() {
    return { data: this.bytes }
  }

  putImageData(imageData) {
    this.presented.push(imageData)
  }
}

class FakeCanvas {
  constructor(context) {
    this.context = context
    this.width = 0
    this.height = 0
    this.options = []
  }

  getContext(_kind, options) {
    this.options.push(options)
    return this.context
  }
}

function digest(bytes) {
  return createHash('sha256').update(bytes).digest('hex')
}

function createRuntime(colorModel = ColorModel.RGBA8888) {
  const runtime = {
    HEAPU8: new Uint8Array(2048),
    selectCalls: 0,
    updateCalls: 0,
    mallocCalls: 0,
    _frei0r_demo_catalog_color_model: () => colorModel,
    _frei0r_demo_select: () => {
      runtime.selectCalls += 1
      return 0
    },
    _frei0r_demo_input_pointer: () => 16,
    _frei0r_demo_output_pointer: () => 1024,
    _frei0r_demo_update: () => {
      runtime.updateCalls += 1
      runtime.HEAPU8.copyWithin(1024, 16, 16 + 256)
      return 0
    },
    _frei0r_demo_last_error: () => 0
  }
  return runtime
}

function createPipeline(runtime, bytes) {
  const captureContext = new FakeContext(bytes)
  const outputContext = new FakeContext(bytes)
  const captureCanvas = new FakeCanvas(captureContext)
  const outputCanvas = new FakeCanvas(outputContext)
  let imageDataCreations = 0
  const pipeline = new FramePipeline(runtime, {
    captureCanvas,
    outputCanvas,
    createImageData(data, width, height) {
      imageDataCreations += 1
      return { data, width, height }
    }
  })
  return { pipeline, captureContext, outputContext, captureCanvas, imageDataCreations: () => imageDataCreations }
}

for (const colorModel of [ColorModel.RGBA8888, ColorModel.PACKED32]) {
  test(`preserves deterministic RGBA bytes for color model ${colorModel}`, () => {
    const bytes = Uint8ClampedArray.from({ length: 256 }, (_, index) => (index * 37 + 11) % 256)
    const runtime = createRuntime(colorModel)
    const fixture = createPipeline(runtime, bytes)

    fixture.pipeline.configure(0, { width: 8, height: 8 })
    fixture.pipeline.render({}, 1.5)

    const output = fixture.outputContext.presented[0].data
    assert.equal(digest(output), digest(bytes))
    assert.deepEqual([...output.subarray(0, 8)], [...bytes.subarray(0, 8)])
    assert.equal(runtime.updateCalls, 1)
    assert.equal(fixture.captureCanvas.options[0].willReadFrequently, true)
    assert.equal(fixture.captureCanvas.options[0].colorSpace, 'srgb')
  })
}

test('refreshes cached Wasm views only after memory growth', () => {
  const bytes = Uint8ClampedArray.from({ length: 256 }, (_, index) => index)
  const runtime = createRuntime()
  const fixture = createPipeline(runtime, bytes)
  fixture.pipeline.configure(0, { width: 8, height: 8 })
  fixture.pipeline.render({}, 0)
  fixture.pipeline.render({}, 1)
  assert.equal(fixture.imageDataCreations(), 1)
  assert.equal(runtime.mallocCalls, 0)

  runtime.HEAPU8 = new Uint8Array(4096)
  fixture.pipeline.render({}, 2)
  assert.equal(fixture.imageDataCreations(), 2)
  assert.equal(runtime.mallocCalls, 0)
})

test('rejects BGRA filters before constructing a runtime instance', () => {
  const runtime = createRuntime(ColorModel.BGRA8888)
  const fixture = createPipeline(runtime, new Uint8ClampedArray(256))

  assert.throws(() => fixture.pipeline.configure(0, { width: 8, height: 8 }), /unsupported pixel format/)
  assert.equal(runtime.selectCalls, 0)
})
