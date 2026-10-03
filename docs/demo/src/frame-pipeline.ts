import type { Frei0rDemoRuntime } from './runtime'

export const ColorModel = {
  BGRA8888: 0,
  RGBA8888: 1,
  PACKED32: 2
} as const

const frameAlignment = 16
const maximumPixels = 640 * 480

export interface FrameDimensions {
  readonly width: number
  readonly height: number
}

interface ImageDataLike {
  readonly data: Uint8ClampedArray
}

interface FrameContext {
  drawImage(source: CanvasImageSource, dx: number, dy: number, dw: number, dh: number): void
  getImageData(sx: number, sy: number, sw: number, sh: number): ImageDataLike
  putImageData(imageData: ImageDataLike, dx: number, dy: number): void
}

interface FrameCanvas {
  width: number
  height: number
  getContext(contextId: '2d', options?: CanvasRenderingContext2DSettings): FrameContext | null
}

export interface FramePipelineOptions {
  readonly captureCanvas: FrameCanvas
  readonly outputCanvas: FrameCanvas
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

export function supportsFrameColorModel(colorModel: number): boolean {
  return colorModel === ColorModel.RGBA8888 || colorModel === ColorModel.PACKED32
}

export class FramePipeline {
  private readonly captureContext: FrameContext
  private readonly outputContext: FrameContext
  private readonly createImageData: (data: Uint8ClampedArray, width: number, height: number) => ImageDataLike
  private configured: FrameDimensions | undefined
  private inputPointer = 0
  private outputPointer = 0
  private input: Uint8Array | undefined
  private outputImage: ImageDataLike | undefined
  private heapBuffer: ArrayBufferLike | undefined

  public constructor(
    private readonly runtime: Frei0rDemoRuntime,
    private readonly options: FramePipelineOptions
  ) {
    this.captureContext = this.getSrgbContext(options.captureCanvas, true)
    this.outputContext = this.getSrgbContext(options.outputCanvas, false)
    this.createImageData = options.createImageData ?? createBrowserImageData
  }

  public configure(catalogIndex: number, dimensions: FrameDimensions): void {
    if (!validDimensions(dimensions)) {
      throw new Error('Frame dimensions must be positive multiples of eight within the demo pixel budget.')
    }

    const colorModel = this.runtime._frei0r_demo_catalog_color_model(catalogIndex)
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
    this.inputPointer = this.runtime._frei0r_demo_input_pointer()
    this.outputPointer = this.runtime._frei0r_demo_output_pointer()
    this.heapBuffer = undefined
    this.refreshViews()
  }

  public render(video: CanvasImageSource, time: number): void {
    if (!this.configured) throw new Error('Configure a filter before rendering frames.')
    this.refreshViews()

    const { width, height } = this.configured
    this.captureContext.drawImage(video, 0, 0, width, height)
    this.input?.set(this.captureContext.getImageData(0, 0, width, height).data)
    if (this.runtime._frei0r_demo_update(time) !== 0) {
      throw new Error(`The selected filter could not process this frame (runtime error ${this.runtime._frei0r_demo_last_error?.() ?? 'unknown'}).`)
    }
    this.outputContext.putImageData(this.outputImage!, 0, 0)
  }

  private getSrgbContext(canvas: FrameCanvas, willReadFrequently: boolean): FrameContext {
    const options: CanvasRenderingContext2DSettings = {
      alpha: false,
      colorSpace: 'srgb',
      ...(willReadFrequently ? { willReadFrequently: true } : {})
    }
    return canvas.getContext('2d', options) ??
      canvas.getContext('2d', { alpha: false, ...(willReadFrequently ? { willReadFrequently: true } : {}) }) ??
      (() => { throw new Error('Canvas 2D is unavailable in this browser.') })()
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
