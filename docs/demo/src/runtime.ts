export interface Frei0rDemoRuntime {
  readonly HEAPU8: Uint8Array
  readonly _frei0r_demo_catalog_count: () => number
  readonly _frei0r_demo_catalog_color_model: (catalogIndex: number) => number
  readonly _frei0r_demo_select: (catalogIndex: number, width: number, height: number) => number
  readonly _frei0r_demo_input_pointer: () => number
  readonly _frei0r_demo_output_pointer: () => number
  readonly _frei0r_demo_update: (time: number) => number
  readonly _frei0r_demo_last_error?: () => number
  readonly _frei0r_demo_parameter_count: () => number
  readonly _frei0r_demo_parameter_type: (parameterIndex: number) => number
  readonly _frei0r_demo_parameter_name: (parameterIndex: number) => number
  readonly _frei0r_demo_parameter_explanation: (parameterIndex: number) => number
  readonly _frei0r_demo_get_parameter_scalar: (parameterIndex: number) => number
  readonly _frei0r_demo_set_parameter_scalar: (parameterIndex: number, value: number) => number
  readonly _frei0r_demo_get_parameter_color_component: (parameterIndex: number, component: number) => number
  readonly _frei0r_demo_set_parameter_color: (parameterIndex: number, red: number, green: number, blue: number) => number
  readonly _frei0r_demo_get_parameter_position_component: (parameterIndex: number, component: number) => number
  readonly _frei0r_demo_set_parameter_position: (parameterIndex: number, x: number, y: number) => number
  readonly _frei0r_demo_reset_parameters: () => number
}

interface RuntimeManifest {
  glue: string
  wasm: string
}

const runtimeName = /^frei0r-demo-runtime\.[a-f0-9]{12}\.(?:mjs|wasm)$/
let runtimePromise: Promise<Frei0rDemoRuntime> | undefined

async function readRuntimeManifest(): Promise<RuntimeManifest> {
  const response = await fetch(`${import.meta.env.BASE_URL}runtime/runtime-manifest.json`)
  if (!response.ok) {
    throw new Error(`Unable to load the frei0r runtime manifest (${response.status}).`)
  }

  const manifest = await response.json() as RuntimeManifest
  if (!runtimeName.test(manifest.glue) || !runtimeName.test(manifest.wasm)) {
    throw new Error('The frei0r runtime manifest contains invalid asset names.')
  }

  return manifest
}

export async function initializeFrei0rDemoRuntime(): Promise<Frei0rDemoRuntime> {
  if (!runtimePromise) {
    runtimePromise = createRuntime().catch((error: unknown) => {
      runtimePromise = undefined
      throw error
    })
  }
  return runtimePromise
}

async function createRuntime(): Promise<Frei0rDemoRuntime> {
  const manifest = await readRuntimeManifest()
  const runtimeBase = new URL(`${import.meta.env.BASE_URL}runtime/`, window.location.origin)
  const glueUrl = new URL(manifest.glue, runtimeBase).href
  const wasmUrl = new URL(manifest.wasm, runtimeBase).href
  const runtimeModule = await import(/* @vite-ignore */ glueUrl)

  if (typeof runtimeModule.default !== 'function') {
    throw new Error('The frei0r runtime module does not export its factory.')
  }

  return runtimeModule.default({
    locateFile(file: string) {
      return file.endsWith('.wasm') ? wasmUrl : new URL(file, glueUrl).href
    }
  }) as Promise<Frei0rDemoRuntime>
}
