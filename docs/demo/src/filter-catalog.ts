import type { Frei0rDemoRuntime } from './runtime'

export interface FilterCatalogItem {
  readonly index: number
  readonly id: string
  readonly name: string
  readonly explanation: string
  readonly parameterCount: number
}

function readText(runtime: Frei0rDemoRuntime, pointer: number, label: string): string {
  const heap = runtime.HEAPU8
  if (!Number.isInteger(pointer) || pointer < 0 || pointer >= heap.length) {
    throw new Error(`The runtime returned invalid ${label} metadata.`)
  }
  const end = heap.indexOf(0, pointer)
  if (end < 0) throw new Error(`The runtime returned unterminated ${label} metadata.`)
  return new TextDecoder().decode(heap.subarray(pointer, end))
}

export function readFilterCatalog(runtime: Frei0rDemoRuntime): readonly FilterCatalogItem[] {
  const count = runtime._frei0r_demo_catalog_count()
  if (!Number.isSafeInteger(count) || count < 1) {
    throw new Error('The local runtime did not expose a usable filter catalog.')
  }

  return Array.from({ length: count }, (_, index) => ({
    index,
    id: readText(runtime, runtime._frei0r_demo_catalog_id(index), 'filter identifier'),
    name: readText(runtime, runtime._frei0r_demo_catalog_name(index), 'filter name'),
    explanation: readText(runtime, runtime._frei0r_demo_catalog_explanation(index), 'filter explanation'),
    parameterCount: runtime._frei0r_demo_catalog_parameter_count(index)
  }))
}
