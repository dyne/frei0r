import { existsSync, readFileSync } from 'node:fs'
import { dirname, join } from 'node:path'
import { fileURLToPath } from 'node:url'

const docsDirectory = join(dirname(fileURLToPath(import.meta.url)), '..')
const demoDirectory = join(docsDirectory, '.vitepress', 'dist', 'demo')
const basePath = process.env.BASE_PATH ?? '/frei0r/'
const normalizedBase = basePath === '/'
  ? '/'
  : `/${basePath.replace(/^\/+|\/+$/g, '')}/`
const demoBase = `${normalizedBase}demo/`
const indexPath = join(demoDirectory, 'index.html')

if (!existsSync(indexPath)) {
  throw new Error('Build the demo before checking its output.')
}

const html = readFileSync(indexPath, 'utf8')
const localAssetPaths = [...html.matchAll(/\b(?:href|src)=["']([^"']+)["']/g)]
  .map((match) => match[1])
  .filter((value) => !value.startsWith('data:'))

if (localAssetPaths.length === 0) {
  throw new Error('The demo HTML does not reference any application assets.')
}

for (const assetPath of localAssetPaths) {
  if (!assetPath.startsWith(demoBase)) {
    throw new Error(`Demo asset is outside its configured base: ${assetPath}`)
  }

  const outputPath = join(demoDirectory, assetPath.slice(demoBase.length))
  if (!existsSync(outputPath)) {
    throw new Error(`Demo asset is missing from the build output: ${assetPath}`)
  }
}

const runtimeManifestPath = join(demoDirectory, 'runtime', 'runtime-manifest.json')
if (!existsSync(runtimeManifestPath)) {
  throw new Error('The demo runtime manifest is missing from the build output.')
}

const runtimeManifest = JSON.parse(readFileSync(runtimeManifestPath, 'utf8'))
const runtimeName = /^frei0r-demo-runtime\.[a-f0-9]{12}\.(?:mjs|wasm)$/
if (!runtimeName.test(runtimeManifest.glue) || !runtimeName.test(runtimeManifest.wasm)) {
  throw new Error('The demo runtime manifest does not use cache-busted product names.')
}

const gluePath = join(demoDirectory, 'runtime', runtimeManifest.glue)
const wasmPath = join(demoDirectory, 'runtime', runtimeManifest.wasm)
if (!existsSync(gluePath) || !existsSync(wasmPath)) {
  throw new Error('The demo runtime manifest references missing products.')
}

const glue = readFileSync(gluePath, 'utf8')
const wasm = readFileSync(wasmPath)
if (!glue.includes('export default createFrei0rDemoRuntime') ||
    !wasm.subarray(0, 4).equals(Buffer.from([0x00, 0x61, 0x73, 0x6d]))) {
  throw new Error('The demo runtime products are not matching Emscripten glue and Wasm files.')
}

const applicationBundles = localAssetPaths
  .filter((assetPath) => assetPath.endsWith('.js'))
  .map((assetPath) => readFileSync(join(demoDirectory, assetPath.slice(demoBase.length)), 'utf8'))
if (!applicationBundles.some((bundle) =>
  bundle.includes('runtime-manifest.json') &&
  bundle.includes('locateFile') &&
  bundle.includes('service-worker.js'))) {
  throw new Error('The application bundle does not initialize the runtime with locateFile.')
}

const manifestPath = join(demoDirectory, 'manifest.webmanifest')
const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'))
if (manifest.name !== 'frei0r Live Filter Demo' ||
  manifest.short_name !== 'frei0r Demo' ||
  manifest.start_url !== './' ||
  manifest.scope !== './' ||
  manifest.display !== 'standalone' ||
  manifest.background_color !== '#101514' ||
  manifest.theme_color !== '#101514') {
  throw new Error('The demo manifest does not declare its standalone, relative shell.')
}

for (const [icon, dimensions] of [
  ['icons/dyne-mark-192.png', 192],
  ['icons/dyne-mark-512.png', 512],
]) {
  if (!manifest.icons.some((entry) => entry.src === icon && entry.sizes === `${dimensions}x${dimensions}`)) {
    throw new Error(`The demo manifest does not declare its ${dimensions}px icon.`)
  }
  const png = readFileSync(join(demoDirectory, icon))
  if (!png.subarray(0, 8).equals(Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a])) ||
    png.readUInt32BE(16) !== dimensions || png.readUInt32BE(20) !== dimensions) {
    throw new Error(`The demo ${dimensions}px icon is not a ${dimensions}px PNG.`)
  }
}

const serviceWorkerPath = join(demoDirectory, 'service-worker.js')
const serviceWorker = readFileSync(serviceWorkerPath, 'utf8')
if (!serviceWorker.includes('const scopePath = new URL(self.registration.scope).pathname') ||
  !serviceWorker.includes('!url.pathname.startsWith(scopePath)') ||
  !serviceWorker.includes('cache.addAll(precache)') ||
  !serviceWorker.includes(runtimeManifest.glue) ||
  !serviceWorker.includes(runtimeManifest.wasm)) {
  throw new Error('The demo service worker is not constrained to its scoped, hashed shell.')
}

const textOutputFiles = [indexPath, gluePath, serviceWorkerPath]
for (const assetPath of localAssetPaths) {
  textOutputFiles.push(join(demoDirectory, assetPath.slice(demoBase.length)))
}
for (const outputFile of textOutputFiles) {
  if (readFileSync(outputFile, 'utf8').includes('/home/gestalt/devel/dyne/frei0r/')) {
    throw new Error(`Build-tree path leaked into ${outputFile}`)
  }
}
if (wasm.includes(Buffer.from('/home/gestalt/devel/dyne/frei0r/'))) {
  throw new Error('Build-tree path leaked into the demo Wasm binary.')
}

console.log(`Checked ${localAssetPaths.length} app assets and cache-busted Wasm runtime under ${demoBase}`)
