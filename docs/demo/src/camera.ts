export type CameraStatus = 'idle' | 'starting' | 'active' | 'error'

export type CameraErrorCategory =
  | 'unsupported'
  | 'permission-denied'
  | 'no-device'
  | 'cancelled'
  | 'device-lost'
  | 'unknown'

export interface CameraError {
  readonly category: CameraErrorCategory
  readonly message: string
}

export interface CameraTrack {
  readonly readyState?: string
  stop(): void
  addEventListener?(type: 'ended', listener: () => void, options?: AddEventListenerOptions | boolean): void
  removeEventListener?(type: 'ended', listener: () => void): void
}

export interface CameraStream {
  getTracks(): CameraTrack[]
}

export interface CameraPlatform {
  getUserMedia?: (constraints: MediaStreamConstraints) => Promise<CameraStream>
  addPageHideListener?: (listener: () => void) => void
  removePageHideListener?: (listener: () => void) => void
}

export interface CameraSnapshot {
  readonly status: CameraStatus
  readonly stream?: CameraStream
  readonly error?: CameraError
}

export type CameraStartResult =
  | { readonly ok: true; readonly stream: CameraStream }
  | { readonly ok: false; readonly error: CameraError }

const errorMessages: Record<CameraErrorCategory, string> = {
  unsupported: 'This browser cannot access a camera.',
  'permission-denied': 'Camera permission was denied.',
  'no-device': 'No usable camera is available.',
  cancelled: 'Camera startup was cancelled.',
  'device-lost': 'The active camera is no longer available.',
  unknown: 'The camera could not be started.'
}

function cameraError(category: CameraErrorCategory): CameraError {
  return { category, message: errorMessages[category] }
}

function defaultPlatform(): CameraPlatform {
  const mediaDevices = globalThis.navigator?.mediaDevices
  const browserWindow = globalThis.window

  return {
    getUserMedia: mediaDevices?.getUserMedia?.bind(mediaDevices),
    addPageHideListener: browserWindow?.addEventListener?.bind(browserWindow, 'pagehide'),
    removePageHideListener: browserWindow?.removeEventListener?.bind(browserWindow, 'pagehide')
  }
}

function errorCategory(error: unknown): CameraErrorCategory {
  if (error instanceof DOMException) {
    if (error.name === 'NotAllowedError' || error.name === 'SecurityError') {
      return 'permission-denied'
    }
    if (error.name === 'NotFoundError' || error.name === 'OverconstrainedError') {
      return 'no-device'
    }
  }

  const name = typeof error === 'object' && error !== null && 'name' in error
    ? String(error.name)
    : ''
  if (name === 'NotAllowedError' || name === 'SecurityError') return 'permission-denied'
  if (name === 'NotFoundError' || name === 'OverconstrainedError') return 'no-device'
  return 'unknown'
}

export class CameraService {
  private activeStream: CameraStream | undefined
  private startToken = 0
  private destroyed = false
  private snapshotValue: CameraSnapshot = { status: 'idle' }
  private readonly listeners = new Set<(snapshot: CameraSnapshot) => void>()
  private readonly trackListeners = new Map<CameraTrack, () => void>()
  private readonly onPageHide = () => this.stop()

  public constructor(private readonly platform: CameraPlatform = defaultPlatform()) {
    this.platform.addPageHideListener?.(this.onPageHide)
  }

  public get snapshot(): CameraSnapshot {
    return this.snapshotValue
  }

  public subscribe(listener: (snapshot: CameraSnapshot) => void): () => void {
    this.listeners.add(listener)
    listener(this.snapshotValue)
    return () => this.listeners.delete(listener)
  }

  public async start(): Promise<CameraStartResult> {
    this.stopCurrentStream()
    const token = ++this.startToken
    this.publish({ status: 'starting' })

    if (this.destroyed || !this.platform.getUserMedia) {
      return this.finishFailure(token, this.destroyed ? 'cancelled' : 'unsupported')
    }

    try {
      const stream = await this.platform.getUserMedia({ audio: false, video: true })
      if (!this.isCurrent(token)) {
        this.stopStream(stream)
        return { ok: false, error: cameraError('cancelled') }
      }

      if (stream.getTracks().length === 0) {
        this.stopStream(stream)
        return this.finishFailure(token, 'no-device')
      }

      this.activeStream = stream
      this.watchTracks(stream)
      this.publish({ status: 'active', stream })
      return { ok: true, stream }
    } catch (error) {
      return this.finishFailure(token, this.isCurrent(token) ? errorCategory(error) : 'cancelled')
    }
  }

  public stop(): void {
    ++this.startToken
    this.stopCurrentStream()
    if (!this.destroyed) this.publish({ status: 'idle' })
  }

  public destroy(): void {
    if (this.destroyed) return
    this.stop()
    this.destroyed = true
    this.platform.removePageHideListener?.(this.onPageHide)
    this.listeners.clear()
  }

  private isCurrent(token: number): boolean {
    return !this.destroyed && token === this.startToken
  }

  private finishFailure(token: number, category: CameraErrorCategory): CameraStartResult {
    const failure = cameraError(category)
    if (this.isCurrent(token)) this.publish({ status: 'error', error: failure })
    return { ok: false, error: failure }
  }

  private watchTracks(stream: CameraStream): void {
    for (const track of stream.getTracks()) {
      const onEnded = () => this.handleTrackEnded(stream)
      track.addEventListener?.('ended', onEnded, { once: true })
      this.trackListeners.set(track, onEnded)
    }
  }

  private handleTrackEnded(stream: CameraStream): void {
    if (stream !== this.activeStream) return
    this.stopCurrentStream()
    this.publish({ status: 'error', error: cameraError('device-lost') })
  }

  private stopCurrentStream(): void {
    const stream = this.activeStream
    this.activeStream = undefined
    this.unwatchTracks()
    if (stream) this.stopStream(stream)
  }

  private unwatchTracks(): void {
    for (const [track, listener] of this.trackListeners) {
      track.removeEventListener?.('ended', listener)
    }
    this.trackListeners.clear()
  }

  private stopStream(stream: CameraStream): void {
    for (const track of stream.getTracks()) track.stop()
  }

  private publish(snapshot: CameraSnapshot): void {
    this.snapshotValue = snapshot
    for (const listener of this.listeners) listener(snapshot)
  }
}
