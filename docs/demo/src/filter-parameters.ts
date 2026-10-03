import type { Frei0rDemoRuntime } from './runtime'

export const ParameterType = {
  BOOL: 0,
  DOUBLE: 1,
  COLOR: 2,
  POSITION: 3
} as const

export type ParameterValue = boolean | number | readonly [number, number, number] | readonly [number, number]

interface ParameterBase {
  readonly index: number
  readonly name: string
  readonly explanation: string
}

export type FilterParameter =
  | (ParameterBase & { readonly kind: 'boolean'; readonly value: boolean })
  | (ParameterBase & { readonly kind: 'number'; readonly value: number })
  | (ParameterBase & { readonly kind: 'color'; readonly value: readonly [number, number, number] })
  | (ParameterBase & { readonly kind: 'position'; readonly value: readonly [number, number] })

function normalized(value: number): number {
  if (!Number.isFinite(value) || value < 0 || value > 1) {
    throw new Error('frei0r parameter values must be normalized between zero and one.')
  }
  return value
}

export class FilterParameters {
  private parameters: readonly FilterParameter[] = []
  private readonly decoder = new TextDecoder()
  private readonly listeners = new Set<(parameters: readonly FilterParameter[]) => void>()

  public constructor(private readonly runtime: Frei0rDemoRuntime) {}

  public get values(): readonly FilterParameter[] {
    return this.parameters
  }

  public subscribe(listener: (parameters: readonly FilterParameter[]) => void): () => void {
    this.listeners.add(listener)
    listener(this.parameters)
    return () => this.listeners.delete(listener)
  }

  public refresh(): readonly FilterParameter[] {
    const parameters: FilterParameter[] = []
    for (let index = 0; index < this.runtime._frei0r_demo_parameter_count(); ++index) {
      const parameter = this.read(index)
      if (parameter) parameters.push(parameter)
    }
    this.parameters = parameters
    this.publish()
    return this.parameters
  }

  public set(index: number, value: ParameterValue): void {
    const parameter = this.parameters.find((candidate) => candidate.index === index)
    if (!parameter) throw new Error('The selected parameter is not exposed by this filter.')

    let result: number
    if (parameter.kind === 'boolean') {
      if (typeof value !== 'boolean') throw new Error('This parameter expects a boolean value.')
      result = this.runtime._frei0r_demo_set_parameter_scalar(index, value ? 1 : 0)
    } else if (parameter.kind === 'number') {
      if (typeof value !== 'number') throw new Error('This parameter expects a normalized number.')
      result = this.runtime._frei0r_demo_set_parameter_scalar(index, normalized(value))
    } else if (parameter.kind === 'color') {
      if (!Array.isArray(value) || value.length !== 3) throw new Error('This parameter expects an RGB color.')
      result = this.runtime._frei0r_demo_set_parameter_color(index, normalized(value[0]), normalized(value[1]), normalized(value[2]))
    } else {
      if (!Array.isArray(value) || value.length !== 2) throw new Error('This parameter expects a normalized position.')
      result = this.runtime._frei0r_demo_set_parameter_position(index, normalized(value[0]), normalized(value[1]))
    }
    this.check(result)
    this.refresh()
  }

  public reset(): void {
    this.check(this.runtime._frei0r_demo_reset_parameters())
    this.refresh()
  }

  private read(index: number): FilterParameter | undefined {
    const type = this.runtime._frei0r_demo_parameter_type(index)
    const base = {
      index,
      name: this.readText(this.runtime._frei0r_demo_parameter_name(index)),
      explanation: this.readText(this.runtime._frei0r_demo_parameter_explanation(index))
    }
    if (type === ParameterType.BOOL) {
      return { ...base, kind: 'boolean', value: this.runtime._frei0r_demo_get_parameter_scalar(index) >= 0.5 }
    }
    if (type === ParameterType.DOUBLE) {
      return { ...base, kind: 'number', value: this.runtime._frei0r_demo_get_parameter_scalar(index) }
    }
    if (type === ParameterType.COLOR) {
      return {
        ...base,
        kind: 'color',
        value: [
          this.runtime._frei0r_demo_get_parameter_color_component(index, 0),
          this.runtime._frei0r_demo_get_parameter_color_component(index, 1),
          this.runtime._frei0r_demo_get_parameter_color_component(index, 2)
        ]
      }
    }
    if (type === ParameterType.POSITION) {
      return {
        ...base,
        kind: 'position',
        value: [
          this.runtime._frei0r_demo_get_parameter_position_component(index, 0),
          this.runtime._frei0r_demo_get_parameter_position_component(index, 1)
        ]
      }
    }
    return undefined
  }

  private readText(pointer: number): string {
    const heap = this.runtime.HEAPU8
    if (!Number.isInteger(pointer) || pointer < 0 || pointer >= heap.length) {
      throw new Error('The runtime returned invalid parameter metadata.')
    }
    const end = heap.indexOf(0, pointer)
    if (end < 0) throw new Error('The runtime returned unterminated parameter metadata.')
    return this.decoder.decode(heap.subarray(pointer, end))
  }

  private check(result: number): void {
    if (result !== 0) {
      throw new Error(`The runtime rejected this parameter change (${this.runtime._frei0r_demo_last_error?.() ?? 'unknown'}).`)
    }
  }

  private publish(): void {
    for (const listener of this.listeners) listener(this.parameters)
  }
}
