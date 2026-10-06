import type { FilterParameter, ParameterValue } from './filter-parameters'

// Prefer an effect's expressive control over switches and technical options.
const dominantNames: Readonly<Record<string, string>> = {
  colorize: 'hue', distort0r: 'Amplitude', glitch0r: 'Shift intensity',
  pixeliz0r: 'Block width', emboss: 'width45', heatmap0r: 'Grey point',
  vertigo: 'Zoomrate', rgbsplit0r: 'Horizontal split distance',
  filmgrain: 'Grain Amount', glow: 'Blur',
  '3dflippo': 'Z axis rotation', aech0r: 'Fade Factor',
  alpha0ps_alpha0ps: 'Shrink/Grow/Blur amount', alpha0ps_alphagrad: 'Transition width',
  alpha0ps_alphaspot: 'Size X', balanc0r: 'Green Tint', bluescreen0r: 'Distance',
  c0rners: 'Corner 1 X', cluster: 'Num', coloradj_RGB: 'R',
  delay0r: 'DelayTime', denoise_hqdn3d: 'Spatial', edgeglow: 'lupscale',
  elastic_scale: 'Non-Linear Scale Factor', gateweave: 'Maximum Horizontal Movement',
  kaleid0sc0pe: 'segmentation', lenscorrection: 'Correction near edges',
  levels: 'Input black level', lightgraffiti: 'sensitivity', mask0mate: 'Left',
  measure_pr0be: 'Measurement', measure_pr0file: 'Tilt', nosync0r: 'HSync',
  normaliz0r: 'Strength', pixels0rt: 'Threshold', select0r: 'Delta R / A / Hue',
  softglow: 'blur', sopsat: 'saturation', timeout: 'time',
  vignette: 'clearCenter', water: 'distort', '3dperspective': 'yaw',
  tint0r: 'Tint amount'
}

export function dominantParameter(id: string | undefined, parameters: readonly FilterParameter[]) {
  const preferred = id ? dominantNames[id] : undefined
  return parameters.find((parameter) => parameter.name.toLowerCase() === preferred?.toLowerCase())
    ?? parameters.find((parameter) => parameter.kind === 'number' && /amount|strength|intensity|amplitude|size|radius|scale|hue|saturation|contrast|brightness|zoom|blur/i.test(parameter.name))
    ?? parameters.find((parameter) => parameter.kind === 'number')
    ?? parameters.find((parameter) => parameter.kind === 'color' || parameter.kind === 'position')
    ?? parameters[0]
}

export function gestureAxis(dx: number, dy: number): 'horizontal' | 'vertical' | undefined {
  if (Math.max(Math.abs(dx), Math.abs(dy)) < 10) return undefined
  if (Math.abs(dx) > Math.abs(dy) * 1.25) return 'horizontal'
  if (Math.abs(dy) > Math.abs(dx) * 1.25) return 'vertical'
  return undefined
}

export function swipeParameterValue(parameter: FilterParameter, delta: number): ParameterValue {
  const clamp = (value: number) => Math.max(0, Math.min(1, value))
  if (parameter.kind === 'number') return clamp(parameter.value + delta)
  if (parameter.kind === 'boolean') return delta === 0 ? parameter.value : delta > 0
  if (parameter.kind === 'position') return [parameter.value[0], clamp(parameter.value[1] + delta)]
  // Traverse the colour spectrum at full saturation for a visible colour change.
  const [r, g, b] = parameter.value
  const max = Math.max(r, g, b), min = Math.min(r, g, b), chroma = max - min
  let hue = chroma === 0 ? 0 : max === r ? (g - b) / chroma : max === g ? (b - r) / chroma + 2 : (r - g) / chroma + 4
  hue = ((hue / 6 + delta) % 1 + 1) % 1
  const component = (offset: number) => {
    const k = (offset + hue * 6) % 6
    return 1 - Math.max(0, Math.min(k, 4 - k, 1))
  }
  return [component(5), component(3), component(1)]
}

export function parameterLabel(parameter: FilterParameter): string {
  if (parameter.kind === 'boolean') return parameter.value ? 'On' : 'Off'
  if (parameter.kind === 'number') return parameter.value.toFixed(3)
  if (parameter.kind === 'position') return parameter.value.map((value) => value.toFixed(2)).join(', ')
  return '#' + parameter.value.map((value) => Math.round(value * 255).toString(16).padStart(2, '0')).join('')
}

export function navigateFilterKey(
  key: string,
  focusedIndex: number,
  visibleIndexes: readonly number[],
  selectFilter: (index: number) => void
) {
  const position = visibleIndexes.indexOf(focusedIndex)
  if (position < 0 || visibleIndexes.length === 0) return false

  let nextIndex: number | undefined
  if (key === 'ArrowLeft') {
    nextIndex = visibleIndexes[(position - 1 + visibleIndexes.length) % visibleIndexes.length]
  } else if (key === 'ArrowRight') {
    nextIndex = visibleIndexes[(position + 1) % visibleIndexes.length]
  } else if (key === 'Home') {
    nextIndex = visibleIndexes[0]
  } else if (key === 'End') {
    nextIndex = visibleIndexes[visibleIndexes.length - 1]
  }

  if (nextIndex === undefined) return false
  selectFilter(nextIndex)
  return true
}
