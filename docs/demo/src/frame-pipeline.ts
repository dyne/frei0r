import type { Frei0rDemoRuntime } from './runtime'

export const ColorModel = { BGRA8888: 0, RGBA8888: 1, PACKED32: 2 } as const

const frameAlignment = 16
const maximumPixels = 640 * 480

export interface FrameDimensions {
  readonly width: number
  readonly height: number
}

interface ImageDataLike { readonly data: Uint8ClampedArray }
interface OutputContext { putImageData(imageData: ImageDataLike, dx: number, dy: number): void }
interface CanvasCaptureContext {
  drawImage(source: CanvasImageSource, dx: number, dy: number, dw: number, dh: number): void
  getImageData(sx: number, sy: number, sw: number, sh: number): ImageDataLike
}
interface FrameCanvas {
  width: number
  height: number
  getContext(contextId: string, options?: unknown): unknown
}

export interface FrameCapture {
  readonly mode: 'webgl' | 'canvas2d'
  configure(width: number, height: number): void
  copy(source: CanvasImageSource, destination: Uint8Array, width: number, height: number): number
}

export interface FramePipelineSnapshot {
  readonly captureMode: FrameCapture['mode']
  readonly allocatedInputFrames: number
  readonly allocatedInputBytes: number
}

export interface FramePipelineOptions {
  readonly captureCanvas: FrameCanvas
  readonly outputCanvas: FrameCanvas
  readonly capture?: FrameCapture
  readonly createImageData?: (data: Uint8ClampedArray, width: number, height: number) => ImageDataLike
}

function createBrowserImageData(data: Uint8ClampedArray, width: number, height: number): ImageDataLike {
  return new ImageData(data as Uint8ClampedArray<ArrayBuffer>, width, height)
}

function validDimensions({ width, height }: FrameDimensions): boolean {
  return Number.isInteger(width) && Number.isInteger(height) &&
    width >= 8 && height >= 8 && width % 8 === 0 && height % 8 === 0 &&
    width * height <= maximumPixels
}

class CanvasFrameCapture implements FrameCapture {
  public readonly mode = 'canvas2d' as const
  private readonly context: CanvasCaptureContext

  public constructor(canvas: FrameCanvas) {
    this.context = canvas.getContext('2d', {
      alpha: false, colorSpace: 'srgb', willReadFrequently: true
    }) as CanvasCaptureContext | null ??
      canvas.getContext('2d', { alpha: false, willReadFrequently: true }) as CanvasCaptureContext | null ??
      (() => { throw new Error('A reusable frame capture API and Canvas 2D are unavailable in this browser.') })()
  }

  public configure(): void {}

  public copy(source: CanvasImageSource, destination: Uint8Array, width: number, height: number): number {
    this.context.drawImage(source, 0, 0, width, height)
    const frame = this.context.getImageData(0, 0, width, height)
    destination.set(frame.data)
    return frame.data.byteLength
  }
}

class WebGlFrameCapture implements FrameCapture {
  public readonly mode = 'webgl' as const
  private readonly program: WebGLProgram
  private readonly positions: WebGLBuffer
  private readonly texture: WebGLTexture
  private readonly positionLocation: number
  private configuredWidth = 0
  private configuredHeight = 0
  private textureWidth = 0
  private textureHeight = 0

  private constructor(private readonly gl: WebGLRenderingContext) {
    const vertexShader = this.compileShader(gl.VERTEX_SHADER, `
      attribute vec2 position;
      varying vec2 textureCoordinate;
      void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        textureCoordinate = position * 0.5 + 0.5;
      }
    `)
    const fragmentShader = this.compileShader(gl.FRAGMENT_SHADER, `
      precision mediump float;
      uniform sampler2D frame;
      varying vec2 textureCoordinate;
      void main() { gl_FragColor = texture2D(frame, textureCoordinate); }
    `)
    const program = gl.createProgram()
    if (!program) throw new Error('WebGL could not create the frame capture program.')
    gl.attachShader(program, vertexShader)
    gl.attachShader(program, fragmentShader)
    gl.linkProgram(program)
    gl.deleteShader(vertexShader)
    gl.deleteShader(fragmentShader)
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
      throw new Error(`WebGL could not link the frame capture program: ${gl.getProgramInfoLog(program) ?? 'unknown error'}`)
    }

    const positions = gl.createBuffer()
    const texture = gl.createTexture()
    const positionLocation = gl.getAttribLocation(program, 'position')
    if (!positions || !texture || positionLocation < 0) {
      throw new Error('WebGL could not allocate the frame capture resources.')
    }
    this.program = program
    this.positions = positions
    this.texture = texture
    this.positionLocation = positionLocation

    gl.bindBuffer(gl.ARRAY_BUFFER, positions)
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW)
    gl.bindTexture(gl.TEXTURE_2D, texture)
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST)
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST)
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE)
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE)
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, 0)
  }

  public static create(canvas: FrameCanvas): WebGlFrameCapture | undefined {
    const options: WebGLContextAttributes = {
      alpha: false, antialias: false, depth: false, preserveDrawingBuffer: false,
      premultipliedAlpha: false, stencil: false
    }
    const gl = canvas.getContext('webgl2', options) ?? canvas.getContext('webgl', options)
    return gl ? new WebGlFrameCapture(gl as WebGLRenderingContext) : undefined
  }

  public configure(width: number, height: number): void {
    this.configuredWidth = width
    this.configuredHeight = height
    this.textureWidth = 0
    this.textureHeight = 0
    this.gl.viewport(0, 0, width, height)
  }

  public copy(source: CanvasImageSource, destination: Uint8Array, width: number, height: number): number {
    const gl = this.gl
    gl.viewport(0, 0, width, height)
    gl.useProgram(this.program)
    gl.bindBuffer(gl.ARRAY_BUFFER, this.positions)
    gl.enableVertexAttribArray(this.positionLocation)
    gl.vertexAttribPointer(this.positionLocation, 2, gl.FLOAT, false, 0, 0)
    gl.activeTexture(gl.TEXTURE0)
    gl.bindTexture(gl.TEXTURE_2D, this.texture)
    const { width: sourceWidth, height: sourceHeight } = this.sourceDimensions(source)
    if (sourceWidth !== this.textureWidth || sourceHeight !== this.textureHeight) {
      gl.texImage2D(
        gl.TEXTURE_2D, 0, gl.RGBA, sourceWidth, sourceHeight, 0,
        gl.RGBA, gl.UNSIGNED_BYTE, null
      )
      this.textureWidth = sourceWidth
      this.textureHeight = sourceHeight
    }
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl.RGBA, gl.UNSIGNED_BYTE, source as TexImageSource)
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4)
    gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, destination)
    return 0
  }

  private sourceDimensions(source: CanvasImageSource): FrameDimensions {
    const dimensions = source as {
      readonly videoWidth?: number
      readonly videoHeight?: number
      readonly naturalWidth?: number
      readonly naturalHeight?: number
      readonly width?: number
      readonly height?: number
    }
    const width = dimensions.videoWidth ?? dimensions.naturalWidth ?? dimensions.width ?? this.configuredWidth
    const height = dimensions.videoHeight ?? dimensions.naturalHeight ?? dimensions.height ?? this.configuredHeight
    return {
      width: Number.isFinite(width) && width > 0 ? Math.floor(width) : this.configuredWidth,
      height: Number.isFinite(height) && height > 0 ? Math.floor(height) : this.configuredHeight
    }
  }

  private compileShader(type: number, source: string): WebGLShader {
    const shader = this.gl.createShader(type)
    if (!shader) throw new Error('WebGL could not create a frame capture shader.')
    this.gl.shaderSource(shader, source)
    this.gl.compileShader(shader)
    if (!this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)) {
      throw new Error(`WebGL could not compile a frame capture shader: ${this.gl.getShaderInfoLog(shader) ?? 'unknown error'}`)
    }
    return shader
  }
}

function createFrameCapture(canvas: FrameCanvas): FrameCapture {
  return WebGlFrameCapture.create(canvas) ?? new CanvasFrameCapture(canvas)
}

export function supportsFrameColorModel(colorModel: number): boolean {
  return colorModel === ColorModel.BGRA8888 || colorModel === ColorModel.RGBA8888 || colorModel === ColorModel.PACKED32
}

export class FramePipeline {
  private readonly capture: FrameCapture
  private readonly outputContext: OutputContext
  private readonly createImageData: (data: Uint8ClampedArray, width: number, height: number) => ImageDataLike
  private configured: FrameDimensions | undefined
  private inputPointer = 0
  private outputPointer = 0
  private input: Uint8Array | undefined
  private outputImage: ImageDataLike | undefined
  private heapBuffer: ArrayBufferLike | undefined
  private allocatedInputFrames = 0
  private allocatedInputBytes = 0
  private bgra = false

  public constructor(private readonly runtime: Frei0rDemoRuntime, private readonly options: FramePipelineOptions) {
    this.capture = options.capture ?? createFrameCapture(options.captureCanvas)
    this.outputContext = options.outputCanvas.getContext('2d', { alpha: false, colorSpace: 'srgb' }) as OutputContext | null ??
      options.outputCanvas.getContext('2d', { alpha: false }) as OutputContext | null ??
      (() => { throw new Error('Canvas 2D is unavailable in this browser.') })()
    this.createImageData = options.createImageData ?? createBrowserImageData
  }

  public get snapshot(): FramePipelineSnapshot {
    return {
      captureMode: this.capture.mode,
      allocatedInputFrames: this.allocatedInputFrames,
      allocatedInputBytes: this.allocatedInputBytes
    }
  }

  public configure(catalogIndex: number, dimensions: FrameDimensions): void {
    if (!validDimensions(dimensions)) {
      throw new Error('Frame dimensions must be positive multiples of eight within the demo pixel budget.')
    }
    const colorModel = this.runtime._frei0r_demo_catalog_color_model(catalogIndex)
    this.bgra = colorModel === ColorModel.BGRA8888
    if (!supportsFrameColorModel(colorModel)) {
      throw new Error('This filter uses an unsupported pixel format for the browser demo.')
    }
    if (this.runtime._frei0r_demo_select(catalogIndex, dimensions.width, dimensions.height) !== 0) {
      throw new Error(`Unable to configure the selected filter (runtime error ${this.runtime._frei0r_demo_last_error?.() ?? 'unknown'}).`)
    }

    this.configured = dimensions
    this.options.captureCanvas.width = dimensions.width
    this.options.captureCanvas.height = dimensions.height
    this.options.outputCanvas.width = dimensions.width
    this.options.outputCanvas.height = dimensions.height
    this.capture.configure(dimensions.width, dimensions.height)
    this.inputPointer = this.runtime._frei0r_demo_input_pointer()
    this.outputPointer = this.runtime._frei0r_demo_output_pointer()
    this.heapBuffer = undefined
    this.refreshViews()
  }

  public render(video: CanvasImageSource, timeSeconds: number): void {
    if (!this.configured) throw new Error('Configure a filter before rendering frames.')
    this.refreshViews()
    const { width, height } = this.configured
    const allocatedBytes = this.capture.copy(video, this.input!, width, height)
    if (allocatedBytes > 0) {
      ++this.allocatedInputFrames
      this.allocatedInputBytes += allocatedBytes
    }
    if (this.bgra) this.swapRedBlue(this.input!)
    if (this.runtime._frei0r_demo_update(timeSeconds) !== 0) {
      throw new Error(`The selected filter could not process this frame (runtime error ${this.runtime._frei0r_demo_last_error?.() ?? 'unknown'}).`)
    }
    if (this.bgra) this.swapRedBlue(this.outputImage!.data)
    this.outputContext.putImageData(this.outputImage!, 0, 0)
  }

  private swapRedBlue(bytes: Uint8Array | Uint8ClampedArray): void {
    for (let index = 0; index < bytes.length; index += 4) {
      const red = bytes[index]
      bytes[index] = bytes[index + 2]
      bytes[index + 2] = red
    }
  }

  private refreshViews(): void {
    if (!this.configured) return
    const heap = this.runtime.HEAPU8
    if (this.heapBuffer === heap.buffer && this.input && this.outputImage) return
    const byteLength = this.configured.width * this.configured.height * 4
    if (this.inputPointer % frameAlignment !== 0 || this.outputPointer % frameAlignment !== 0 ||
      this.inputPointer + byteLength > heap.byteLength || this.outputPointer + byteLength > heap.byteLength) {
      throw new Error('The runtime returned an invalid frame buffer.')
    }
    this.heapBuffer = heap.buffer
    this.input = heap.subarray(this.inputPointer, this.inputPointer + byteLength)
    const output = new Uint8ClampedArray(heap.buffer, heap.byteOffset + this.outputPointer, byteLength)
    this.outputImage = this.createImageData(output, this.configured.width, this.configured.height)
  }
}
