import type { FrameDimensions } from './frame-pipeline'

const maximumPixels = 640 * 480
const minimumDimension = 8
const dimensionStep = 8

export interface FrameRenderer {
  configure(catalogIndex: number, dimensions: FrameDimensions): void
  render(video: CanvasImageSource, time: number): void
}

export interface FrameVideo {
  readonly videoWidth: number
  readonly videoHeight: number
  requestVideoFrameCallback?(callback: (now: number, metadata: unknown) => void): number
  cancelVideoFrameCallback?(handle: number): void
}

export interface FrameSchedulerPlatform {
  requestAnimationFrame(callback: FrameRequestCallback): number
  cancelAnimationFrame(handle: number): void
  now(): number
}

export interface FrameSchedulerSnapshot {
  readonly active: boolean
  readonly status: string
  readonly renderedFrames: number
  readonly droppedFrames: number
  readonly qualityScale: number
  readonly selectedFilter?: number
  readonly pendingFilter?: number
  readonly dimensions?: FrameDimensions
}

interface ScheduledCallback {
  readonly kind: 'video' | 'animation'
  readonly handle: number
}

function browserPlatform(): FrameSchedulerPlatform {
  return {
    requestAnimationFrame: globalThis.requestAnimationFrame.bind(globalThis),
    cancelAnimationFrame: globalThis.cancelAnimationFrame.bind(globalThis),
    now: () => performance.now()
  }
}

export function deriveFrameDimensions(
  sourceWidth: number,
  sourceHeight: number,
  pixelBudget = maximumPixels
): FrameDimensions | undefined {
  if (!Number.isFinite(sourceWidth) || !Number.isFinite(sourceHeight) ||
    sourceWidth <= 0 || sourceHeight <= 0 || pixelBudget < minimumDimension ** 2) return undefined

  const scale = Math.min(1, Math.sqrt(pixelBudget / (sourceWidth * sourceHeight)))
  let width = Math.max(minimumDimension, Math.floor((sourceWidth * scale) / dimensionStep) * dimensionStep)
  let height = Math.max(minimumDimension, Math.floor((sourceHeight * scale) / dimensionStep) * dimensionStep)

  while (width * height > pixelBudget && (width > minimumDimension || height > minimumDimension)) {
    if (width >= height && width > minimumDimension) width -= dimensionStep
    else if (height > minimumDimension) height -= dimensionStep
  }
  return { width, height }
}

export class FrameScheduler {
  private active = false
  private inFlight = false
  private selectedFilter: number | undefined
  private pendingFilter: number | undefined
  private dimensions: FrameDimensions | undefined
  private pendingDimensions: FrameDimensions | undefined
  private scheduled: ScheduledCallback | undefined
  private renderedFrames = 0
  private droppedFrames = 0
  private qualityScale = 1
  private status = 'Camera processing is stopped.'
  private readonly listeners = new Set<(snapshot: FrameSchedulerSnapshot) => void>()
  private readonly configurationListeners = new Set<(catalogIndex: number, dimensions: FrameDimensions) => void>()

  public constructor(
    private readonly renderer: FrameRenderer,
    private readonly video: FrameVideo,
    private readonly platform: FrameSchedulerPlatform = browserPlatform(),
    private readonly longTaskMilliseconds = 50
  ) {}

  public get snapshot(): FrameSchedulerSnapshot {
    return {
      active: this.active,
      status: this.status,
      renderedFrames: this.renderedFrames,
      droppedFrames: this.droppedFrames,
      qualityScale: this.qualityScale,
      ...(this.selectedFilter === undefined ? {} : { selectedFilter: this.selectedFilter }),
      ...(this.pendingFilter === undefined ? {} : { pendingFilter: this.pendingFilter }),
      ...(this.dimensions ? { dimensions: this.dimensions } : {})
    }
  }

  public subscribe(listener: (snapshot: FrameSchedulerSnapshot) => void): () => void {
    this.listeners.add(listener)
    listener(this.snapshot)
    return () => this.listeners.delete(listener)
  }

  public subscribeConfiguration(listener: (catalogIndex: number, dimensions: FrameDimensions) => void): () => void {
    this.configurationListeners.add(listener)
    return () => this.configurationListeners.delete(listener)
  }

  public selectFilter(catalogIndex: number): void {
    if (!Number.isInteger(catalogIndex) || catalogIndex < 0) {
      throw new Error('The selected filter index must be a non-negative integer.')
    }
    this.pendingFilter = catalogIndex
    this.status = 'Applying filter change after the current frame.'
    this.publish()
  }

  public start(): void {
    if (this.active) return
    this.active = true
    this.status = 'Waiting for a camera frame.'
    this.publish()
    this.scheduleNext()
  }

  public stop(): void {
    this.active = false
    this.cancelScheduledCallback()
    this.status = 'Camera processing is stopped.'
    this.publish()
  }

  public receiveFrame(time: number): void {
    if (!this.active) return
    if (this.inFlight) {
      ++this.droppedFrames
      this.status = 'Dropping a camera frame while processing is busy.'
      this.publish()
      return
    }

    const nextDimensions = deriveFrameDimensions(
      this.video.videoWidth,
      this.video.videoHeight,
      Math.floor(maximumPixels * this.qualityScale)
    )
    if (!nextDimensions) {
      this.status = 'Waiting for camera dimensions.'
      this.publish()
      this.scheduleNext()
      return
    }
    if (!this.sameDimensions(nextDimensions, this.dimensions)) this.pendingDimensions = nextDimensions
    if (this.selectedFilter === undefined && this.pendingFilter === undefined) {
      this.status = 'Choose a filter to start processing.'
      this.publish()
      this.scheduleNext()
      return
    }

    this.inFlight = true
    const startedAt = this.platform.now()
    try {
      this.applyPendingChanges(nextDimensions)
      this.renderer.render(this.video as unknown as CanvasImageSource, time)
      ++this.renderedFrames
      this.status = 'Processing camera frames locally.'
    } catch (error) {
      this.status = error instanceof Error ? error.message : 'Unable to process this camera frame.'
    } finally {
      this.inFlight = false
      if (this.platform.now() - startedAt > this.longTaskMilliseconds) this.reduceQuality()
      this.publish()
      this.scheduleNext()
    }
  }

  private applyPendingChanges(currentDimensions: FrameDimensions): void {
    const nextFilter = this.pendingFilter ?? this.selectedFilter
    const nextDimensions = this.pendingDimensions ?? currentDimensions
    if (nextFilter === undefined) return
    if (nextFilter !== this.selectedFilter || !this.sameDimensions(nextDimensions, this.dimensions)) {
      this.renderer.configure(nextFilter, nextDimensions)
      this.selectedFilter = nextFilter
      this.dimensions = nextDimensions
      for (const listener of this.configurationListeners) listener(nextFilter, nextDimensions)
    }
    this.pendingFilter = undefined
    this.pendingDimensions = undefined
  }

  private reduceQuality(): void {
    const nextScale = Math.max(0.25, this.qualityScale * 0.75)
    if (nextScale === this.qualityScale) return
    this.qualityScale = nextScale
    this.pendingDimensions = deriveFrameDimensions(
      this.video.videoWidth,
      this.video.videoHeight,
      Math.floor(maximumPixels * this.qualityScale)
    )
    this.status = 'Reducing preview quality to keep processing responsive.'
  }

  private scheduleNext(): void {
    if (!this.active || this.scheduled) return
    if (this.video.requestVideoFrameCallback) {
      const handle = this.video.requestVideoFrameCallback((time) => {
        this.scheduled = undefined
        this.receiveFrame(time)
      })
      this.scheduled = { kind: 'video', handle }
      return
    }
    const handle = this.platform.requestAnimationFrame((time) => {
      this.scheduled = undefined
      this.receiveFrame(time)
    })
    this.scheduled = { kind: 'animation', handle }
  }

  private cancelScheduledCallback(): void {
    if (!this.scheduled) return
    if (this.scheduled.kind === 'video') this.video.cancelVideoFrameCallback?.(this.scheduled.handle)
    else this.platform.cancelAnimationFrame(this.scheduled.handle)
    this.scheduled = undefined
  }

  private sameDimensions(first: FrameDimensions | undefined, second: FrameDimensions | undefined): boolean {
    return first?.width === second?.width && first?.height === second?.height
  }

  private publish(): void {
    const snapshot = this.snapshot
    for (const listener of this.listeners) listener(snapshot)
  }
}
