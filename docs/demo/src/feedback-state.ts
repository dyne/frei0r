export interface FeedbackInputs {
  readonly stageStatus: string
  readonly online: boolean
  readonly qualityScale: number
  readonly rapidVisualChanges: boolean
}

export function qualityMessage(qualityScale: number): string | undefined {
  if (!Number.isFinite(qualityScale) || qualityScale >= 1) return undefined
  return `Preview quality reduced to ${Math.round(qualityScale * 100)}% to keep processing responsive.`
}

export function connectionMessage(online: boolean): string | undefined {
  return online ? undefined : 'You are offline. Cached demo files remain available when already stored on this device.'
}

export function hasRapidVisualChanges(filterId: string | undefined): boolean {
  return filterId === 'glitch0r'
}

export function composeLiveStatus(inputs: FeedbackInputs): string {
  return [
    inputs.stageStatus,
    connectionMessage(inputs.online),
    qualityMessage(inputs.qualityScale),
    inputs.rapidVisualChanges ? 'Rapid visual changes are possible with this filter.' : undefined
  ].filter((message): message is string => Boolean(message)).join(' ')
}
