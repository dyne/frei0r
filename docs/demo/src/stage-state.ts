import type { CameraSnapshot } from './camera'
import type { FrameSchedulerSnapshot } from './frame-scheduler'

export type RuntimeState = 'loading' | 'ready' | 'failure'

export type StageFixture =
  | 'ready'
  | 'requesting'
  | 'running'
  | 'denied'
  | 'unavailable'
  | 'runtime-failure'

export type StageKind =
  | StageFixture
  | 'paused'
  | 'stopped'

export interface StagePresentation {
  readonly kind: StageKind
  readonly title: string
  readonly detail: string
  readonly status: string
  readonly action: 'start' | 'stop' | 'retry' | 'none'
  readonly actionLabel: string
  readonly actionDisabled: boolean
  readonly cameraActive: boolean
  readonly showCanvas: boolean
}

export interface StageInputs {
  readonly camera: CameraSnapshot
  readonly runtimeState: RuntimeState
  readonly runtimeError?: string
  readonly scheduler: Pick<FrameSchedulerSnapshot, 'active' | 'status'>
  readonly cameraWasStarted: boolean
  readonly fixture?: StageFixture
}

const fixtures: Record<StageFixture, StagePresentation> = {
  ready: {
    kind: 'ready',
    title: 'See a frei0r filter on your camera',
    detail: 'Processing stays in this browser. Start only when you are ready to share a camera preview.',
    status: 'Ready to start the camera.',
    action: 'start',
    actionLabel: 'Start camera',
    actionDisabled: false,
    cameraActive: false,
    showCanvas: false
  },
  requesting: {
    kind: 'requesting',
    title: 'Waiting for camera permission',
    detail: 'Choose Allow in your browser prompt to begin the local preview.',
    status: 'Requesting camera permission.',
    action: 'none',
    actionLabel: '',
    actionDisabled: true,
    cameraActive: false,
    showCanvas: false
  },
  running: {
    kind: 'running',
    title: 'Camera preview is active',
    detail: 'Frames are being filtered locally in this browser.',
    status: 'Camera active. Processing frames locally.',
    action: 'stop',
    actionLabel: 'Stop camera',
    actionDisabled: false,
    cameraActive: true,
    showCanvas: true
  },
  denied: {
    kind: 'denied',
    title: 'Camera permission was not granted',
    detail: 'Allow camera access in your browser settings, then try again. No video or audio was uploaded.',
    status: 'Camera permission denied.',
    action: 'retry',
    actionLabel: 'Try camera again',
    actionDisabled: false,
    cameraActive: false,
    showCanvas: false
  },
  unavailable: {
    kind: 'unavailable',
    title: 'No usable camera is available',
    detail: 'Connect or enable a camera, then try again. This demo does not send frames to a server.',
    status: 'Camera unavailable.',
    action: 'retry',
    actionLabel: 'Try camera again',
    actionDisabled: false,
    cameraActive: false,
    showCanvas: false
  },
  'runtime-failure': {
    kind: 'runtime-failure',
    title: 'The local filter runtime could not start',
    detail: 'Reload the demo to retry the local WebAssembly runtime. Camera access has not been requested.',
    status: 'Local filter runtime unavailable.',
    action: 'none',
    actionLabel: '',
    actionDisabled: true,
    cameraActive: false,
    showCanvas: false
  }
}

function pausedPresentation(status: string): StagePresentation {
  return {
    kind: 'paused',
    title: 'Camera preview is paused',
    detail: 'The camera is still active. Processing resumes when a new frame is available.',
    status,
    action: 'stop',
    actionLabel: 'Stop camera',
    actionDisabled: false,
    cameraActive: true,
    showCanvas: true
  }
}

function stoppedPresentation(): StagePresentation {
  return {
    kind: 'stopped',
    title: 'Camera preview stopped',
    detail: 'The camera has been released. Start again whenever you want to apply a local filter.',
    status: 'Camera stopped.',
    action: 'start',
    actionLabel: 'Restart camera',
    actionDisabled: false,
    cameraActive: false,
    showCanvas: false
  }
}

function runtimeLoadingPresentation(): StagePresentation {
  return {
    kind: 'requesting',
    title: 'Preparing local filters',
    detail: 'The frei0r runtime is loading before any camera permission is requested.',
    status: 'Preparing the local WebAssembly runtime.',
    action: 'start',
    actionLabel: 'Preparing camera',
    actionDisabled: true,
    cameraActive: false,
    showCanvas: false
  }
}

export function stageFixtureFromSearch(search: string, enabled: boolean): StageFixture | undefined {
  if (!enabled) return undefined
  const value = new URLSearchParams(search).get('stage')
  return value && value in fixtures ? value as StageFixture : undefined
}

export function resolveStagePresentation(inputs: StageInputs): StagePresentation {
  if (inputs.fixture) return fixtures[inputs.fixture]
  if (inputs.runtimeState === 'failure') {
    return {
      ...fixtures['runtime-failure'],
      detail: inputs.runtimeError
        ? `${inputs.runtimeError} Reload the demo to try again; camera access has not been requested.`
        : fixtures['runtime-failure'].detail
    }
  }
  if (inputs.camera.status === 'starting') return fixtures.requesting
  if (inputs.camera.status === 'active') {
    return inputs.scheduler.active ? fixtures.running : pausedPresentation(inputs.scheduler.status)
  }
  if (inputs.camera.status === 'error') {
    if (inputs.camera.error?.category === 'permission-denied') return fixtures.denied
    if (inputs.camera.error?.category === 'unsupported' || inputs.camera.error?.category === 'no-device' || inputs.camera.error?.category === 'device-lost') {
      return fixtures.unavailable
    }
    return {
      ...fixtures['runtime-failure'],
      title: 'The camera could not start',
      detail: `${inputs.camera.error?.message ?? 'The camera could not be started.'} Try again after checking your camera and browser settings.`,
      status: inputs.camera.error?.message ?? 'Camera could not start.',
      action: 'retry',
      actionLabel: 'Try camera again',
      actionDisabled: false
    }
  }
  if (inputs.runtimeState === 'loading') return runtimeLoadingPresentation()
  return inputs.cameraWasStarted ? stoppedPresentation() : fixtures.ready
}
