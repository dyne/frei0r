import type { FilterCatalogItem } from './filter-catalog'
import type { FilterParameter } from './filter-parameters'

export type ControlFixture = 'zero' | 'maximum' | 'long'
export type FeedbackFixture = 'offline' | 'installable' | 'reduced-quality' | 'flashing'

const fixtureCatalog: readonly FilterCatalogItem[] = [
  { index: 0, id: 'brightness', name: 'Bloom', explanation: 'Lift bright edges into a soft local glow.', parameterCount: 0 },
  { index: 1, id: 'bw0r', name: 'Contrast', explanation: 'Shape the tonal range of the active preview.', parameterCount: 1 },
  { index: 2, id: 'colorize', name: 'Glow', explanation: 'Add a restrained halo around bright details.', parameterCount: 2 },
  { index: 3, id: 'dither', name: 'Kaleidoscope', explanation: 'Mirror the camera frame into repeated facets.', parameterCount: 3 },
  { index: 4, id: 'distort0r', name: 'Lens correction', explanation: 'Correct the apparent shape of the camera lens.', parameterCount: 4 },
  { index: 5, id: 'glitch0r', name: 'Glitch0r', explanation: 'Add local glitches and block shifting.', parameterCount: 5 },
  { index: 6, id: 'glow', name: 'Pixeliz0r', explanation: 'Reduce the preview into distinct pixel blocks.', parameterCount: 6 },
  { index: 7, id: 'heatmap0r', name: 'RGB shift', explanation: 'Offset the red, green, and blue channels.', parameterCount: 7 },
  { index: 8, id: 'hueshift0r', name: 'Saturation', explanation: 'Tune the colour intensity in the live result.', parameterCount: 8 },
  { index: 9, id: 'invert0r', name: 'Scanlines', explanation: 'Lay evenly spaced scanlines over the camera frame.', parameterCount: 9 },
  { index: 10, id: 'pixeliz0r', name: 'Threshold', explanation: 'Separate light and dark areas with a hard edge.', parameterCount: 10 },
  { index: 11, id: 'vertigo', name: 'Three point balance', explanation: 'Balance three local colour reference points.', parameterCount: 11 }
]

const maximumParameters: readonly FilterParameter[] = [
  { index: 0, name: 'Enabled', explanation: 'Turn this component on or off.', kind: 'boolean', value: true },
  { index: 1, name: 'Amount', explanation: 'Set the overall strength.', kind: 'number', value: 0.5 },
  { index: 2, name: 'Red balance', explanation: 'Set the red component.', kind: 'number', value: 0.4 },
  { index: 3, name: 'Green balance', explanation: 'Set the green component.', kind: 'number', value: 0.6 },
  { index: 4, name: 'Blue balance', explanation: 'Set the blue component.', kind: 'number', value: 0.7 },
  { index: 5, name: 'Tint', explanation: 'Choose a local colour tint.', kind: 'color', value: [0.8, 0.3, 0.2] },
  { index: 6, name: 'Center', explanation: 'Choose the effect centre.', kind: 'position', value: [0.5, 0.5] },
  { index: 7, name: 'Scale', explanation: 'Set the size of the effect.', kind: 'number', value: 0.45 },
  { index: 8, name: 'Rotation', explanation: 'Set the turn of the effect.', kind: 'number', value: 0.25 },
  { index: 9, name: 'Softness', explanation: 'Set the transition softness.', kind: 'number', value: 0.65 },
  { index: 10, name: 'Mix', explanation: 'Blend the processed result with the preview.', kind: 'number', value: 0.8 }
]

export function controlFixtureFromSearch(search: string, enabled: boolean): ControlFixture | undefined {
  if (!enabled) return undefined
  const value = new URLSearchParams(search).get('controls')
  return value === 'zero' || value === 'maximum' || value === 'long' ? value : undefined
}

export function feedbackFixtureFromSearch(search: string, enabled: boolean): FeedbackFixture | undefined {
  if (!enabled) return undefined
  const value = new URLSearchParams(search).get('feedback')
  return value === 'offline' || value === 'installable' || value === 'reduced-quality' || value === 'flashing'
    ? value
    : undefined
}

export function fixtureCatalogFor(controlFixture: ControlFixture | undefined): readonly FilterCatalogItem[] {
  if (controlFixture !== 'long') return fixtureCatalog
  return fixtureCatalog.map((item, index) => index === 4 ? {
    ...item,
    name: 'Lens correction for very wide-angle camera sources',
    explanation: 'Correct barrel and perspective distortion in a locally processed camera preview without uploading a frame or interrupting the active result.'
  } : item)
}

export function fixtureParametersFor(controlFixture: ControlFixture | undefined): readonly FilterParameter[] | undefined {
  return controlFixture === 'maximum' ? maximumParameters : controlFixture === 'zero' ? [] : undefined
}
