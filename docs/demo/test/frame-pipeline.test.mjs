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
    return { data: this.bytes.slice() }
  }

  putImageData(imageData) {
    this.presented.push(imageData)
  }
}

class FakeCanvas {
  constructor(context, contextKind = '2d') {
    this.context = context
    this.contextKind = contextKind
    this.width = 0
    this.height = 0
    this.options = []
  }

  getContext(kind, options) {
    this.options.push(options)
    return kind === this.contextKind ? this.context : null
  }
}

class ReusableCapture {
  mode = 'webgl'
  destinations = []

  constructor(bytes) {
    this.bytes = bytes
  }

  configure() {}

  copy(_source, destination) {
    this.destinations.push(destination)
    destination.set(this.bytes)
    return 0
  }
}

class FakeWebGl {
  VERTEX_SHADER = 1
  FRAGMENT_SHADER = 2
  COMPILE_STATUS = 3
  LINK_STATUS = 4
  ARRAY_BUFFER = 5
  STATIC_DRAW = 6
  TEXTURE_2D = 7
  TEXTURE_MIN_FILTER = 8
  TEXTURE_MAG_FILTER = 9
  NEAREST = 10
  TEXTURE_WRAP_S = 11
  TEXTURE_WRAP_T = 12
  CLAMP_TO_EDGE = 13
  UNPACK_FLIP_Y_WEBGL = 14
  FLOAT = 15
  TEXTURE0 = 16
  RGBA = 17
  UNSIGNED_BYTE = 18
  TRIANGLE_STRIP = 19
  storageDefinitions = []
  subImageUploads = []
  readDestinations = []

  createShader() { return {} }
  shaderSource() {}
  compileShader() {}
  getShaderParameter() { return true }
  getShaderInfoLog() { return '' }
  createProgram() { return {} }
  attachShader() {}
  linkProgram() {}
  deleteShader() {}
  getProgramParameter() { return true }
  getProgramInfoLog() { return '' }
  createBuffer() { return {} }
  createTexture() { return {} }
  getAttribLocation() { return 0 }
  bindBuffer() {}
  bufferData() {}
  bindTexture() {}
  texParameteri() {}
  pixelStorei() {}
  viewport() {}
  useProgram() {}
  enableVertexAttribArray() {}
  vertexAttribPointer() {}
  activeTexture() {}
  texImage2D(_target, _level, _internalFormat, width, height) {
    this.storageDefinitions.push({ width, height })
  }
  texSubImage2D(_target, _level, _x, _y, _format, _type, source) {
    this.subImageUploads.push(source)
  }
  drawArrays() {}
  readPixels(_x, _y, _width, _height, _format, _type, destination) {
    this.readDestinations.push(destination)
    for (let index = 0; index < destination.length; ++index) destination[index] = index & 255
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
  const capture = new ReusableCapture(bytes)
  const outputContext = new FakeContext(bytes)
  const captureCanvas = new FakeCanvas(null, 'none')
  const outputCanvas = new FakeCanvas(outputContext)
  let imageDataCreations = 0
  const pipeline = new FramePipeline(runtime, {
    captureCanvas,
    outputCanvas,
    capture,
    createImageData(data, width, height) {
      imageDataCreations += 1
      return { data, width, height }
    }
  })
  return { pipeline, capture, outputContext, captureCanvas, imageDataCreations: () => imageDataCreations }
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
    assert.deepEqual(fixture.pipeline.snapshot, {
      captureMode: 'webgl',
      allocatedInputFrames: 0,
      allocatedInputBytes: 0
    })
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
  assert.equal(fixture.capture.destinations[0], fixture.capture.destinations[1])

  runtime.HEAPU8 = new Uint8Array(4096)
  fixture.pipeline.render({}, 2)
  assert.equal(fixture.imageDataCreations(), 2)
  assert.equal(runtime.mallocCalls, 0)
  assert.notEqual(fixture.capture.destinations[1], fixture.capture.destinations[2])
})

test('accounts for the allocating Canvas 2D fallback on every captured frame', () => {
  const bytes = Uint8ClampedArray.from({ length: 256 }, (_, index) => index)
  const runtime = createRuntime()
  const captureContext = new FakeContext(bytes)
  const outputContext = new FakeContext(bytes)
  const pipeline = new FramePipeline(runtime, {
    captureCanvas: new FakeCanvas(captureContext),
    outputCanvas: new FakeCanvas(outputContext),
    createImageData: (data, width, height) => ({ data, width, height })
  })

  pipeline.configure(0, { width: 8, height: 8 })
  pipeline.render({}, 0)
  pipeline.render({}, 1)

  assert.deepEqual(pipeline.snapshot, {
    captureMode: 'canvas2d',
    allocatedInputFrames: 2,
    allocatedInputBytes: 512
  })
  assert.equal(captureContext.draws, 2)
})

test('defines WebGL texture storage once per configuration and reuses the Wasm destination', () => {
  const runtime = createRuntime()
  const gl = new FakeWebGl()
  const outputContext = new FakeContext(new Uint8ClampedArray(512))
  const pipeline = new FramePipeline(runtime, {
    captureCanvas: new FakeCanvas(gl, 'webgl2'),
    outputCanvas: new FakeCanvas(outputContext),
    createImageData: (data, width, height) => ({ data, width, height })
  })
  const firstSource = { videoWidth: 8, videoHeight: 8 }

  pipeline.configure(0, { width: 8, height: 8 })
  pipeline.render(firstSource, 0)
  pipeline.render(firstSource, 1)

  assert.deepEqual(gl.storageDefinitions, [{ width: 8, height: 8 }])
  assert.equal(gl.subImageUploads.length, 2)
  assert.equal(gl.readDestinations[0], gl.readDestinations[1])
  assert.deepEqual(pipeline.snapshot, {
    captureMode: 'webgl',
    allocatedInputFrames: 0,
    allocatedInputBytes: 0
  })

  const secondSource = { videoWidth: 16, videoHeight: 8 }
  pipeline.configure(0, { width: 16, height: 8 })
  pipeline.render(secondSource, 2)

  assert.deepEqual(gl.storageDefinitions, [
    { width: 8, height: 8 },
    { width: 16, height: 8 }
  ])
  assert.equal(gl.subImageUploads.length, 3)
  assert.notEqual(gl.readDestinations[1], gl.readDestinations[2])
})

test('rejects BGRA filters before constructing a runtime instance', () => {
  const runtime = createRuntime(ColorModel.BGRA8888)
  const fixture = createPipeline(runtime, new Uint8ClampedArray(256))

  assert.throws(() => fixture.pipeline.configure(0, { width: 8, height: 8 }), /unsupported pixel format/)
  assert.equal(runtime.selectCalls, 0)
})
