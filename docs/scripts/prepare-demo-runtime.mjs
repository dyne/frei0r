import { existsSync, mkdirSync, readdirSync, readFileSync, rmSync, statSync, writeFileSync } from 'node:fs'
import { createHash } from 'node:crypto'
import { dirname, join, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'

const docsDirectory = resolve(dirname(fileURLToPath(import.meta.url)), '..')
const projectDirectory = resolve(docsDirectory, '..')
const sourceDirectory = process.env.FREI0R_DEMO_RUNTIME_DIR
const outputDirectory = join(docsDirectory, 'demo', 'public', 'runtime')
const builtRuntimeDirectory = join(docsDirectory, '.vitepress', 'dist', 'demo', 'runtime')
const expectedProducts = [
  'frei0r-demo-runtime.mjs',
  'frei0r-demo-runtime.wasm'
]

if (!sourceDirectory) {
  throw new Error('FREI0R_DEMO_RUNTIME_DIR must name the browser runtime build directory.')
}

const runtimeDirectory = resolve(sourceDirectory)
const sourceProducts = expectedProducts.map((name) => join(runtimeDirectory, name))

for (const product of sourceProducts) {
  if (!existsSync(product) || statSync(product).size === 0) {
    throw new Error(`Missing browser runtime product: ${product}`)
  }
}

const runtimeInputs = readdirSync(join(projectDirectory, 'examples', 'browser-demo', 'runtime'), { withFileTypes: true })
  .filter((entry) => entry.isFile() && /^(?:CMakeLists\.txt|.*\.[ch])$/.test(entry.name))
  .map((entry) => join(projectDirectory, 'examples', 'browser-demo', 'runtime', entry.name))
const oldestProduct = Math.min(...sourceProducts.map((product) => statSync(product).mtimeMs))

if (runtimeInputs.some((input) => statSync(input).mtimeMs > oldestProduct)) {
  throw new Error('Browser runtime products are stale; rebuild frei0r-demo-browser-runtime.')
}

const glueSource = readFileSync(sourceProducts[0], 'utf8')
if (!glueSource.includes('export default createFrei0rDemoRuntime') ||
    !/["']frei0r-demo-runtime\.wasm["']/.test(glueSource)) {
  throw new Error('Browser runtime glue is not the expected Emscripten ES module.')
}

const wasmSource = readFileSync(sourceProducts[1])
if (!wasmSource.subarray(0, 4).equals(Buffer.from([0x00, 0x61, 0x73, 0x6d]))) {
  throw new Error('Browser runtime binary is not a WebAssembly module.')
}

function versionedName(product) {
  const digest = createHash('sha256').update(product.contents).digest('hex').slice(0, 12)
  const extension = product.extension
  return `frei0r-demo-runtime.${digest}.${extension}`
}

function readUnsignedLeb128(bytes, offset) {
  let value = 0
  let shift = 0

  while (offset < bytes.length && shift <= 28) {
    const byte = bytes[offset++]
    value |= (byte & 0x7f) << shift
    if ((byte & 0x80) === 0) {
      return { offset, value }
    }
    shift += 7
  }

  throw new Error('Browser runtime contains an invalid WebAssembly section length.')
}

function stripCustomSections(bytes) {
  const sections = [bytes.subarray(0, 8)]
  let offset = 8

  while (offset < bytes.length) {
    const sectionStart = offset
    const sectionId = bytes[offset++]
    const sectionLength = readUnsignedLeb128(bytes, offset)
    offset = sectionLength.offset + sectionLength.value

    if (offset > bytes.length) {
      throw new Error('Browser runtime contains a truncated WebAssembly section.')
    }
    if (sectionId !== 0) {
      sections.push(bytes.subarray(sectionStart, offset))
    }
  }

  return Buffer.concat(sections)
}

const packagedGlue = readFileSync(sourceProducts[0])
const packagedWasm = stripCustomSections(wasmSource)

rmSync(outputDirectory, { force: true, recursive: true })
rmSync(builtRuntimeDirectory, { force: true, recursive: true })
mkdirSync(outputDirectory, { recursive: true })

const glue = versionedName({ contents: packagedGlue, extension: 'mjs' })
const wasm = versionedName({ contents: packagedWasm, extension: 'wasm' })
writeFileSync(join(outputDirectory, glue), packagedGlue)
writeFileSync(join(outputDirectory, wasm), packagedWasm)
writeFileSync(join(outputDirectory, 'runtime-manifest.json'), `${JSON.stringify({ glue, wasm }, null, 2)}\n`)

console.log(`Prepared frei0r runtime assets: ${glue}, ${wasm}`)
