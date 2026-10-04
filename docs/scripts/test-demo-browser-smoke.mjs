import assert from 'node:assert/strict'
import { createServer } from 'node:http'
import { mkdir, readFile, writeFile } from 'node:fs/promises'
import { extname, join, normalize, resolve, sep } from 'node:path'
import { fileURLToPath } from 'node:url'
import { chromium } from 'playwright'

const docsDirectory = resolve(fileURLToPath(new URL('..', import.meta.url)))
const outputDirectory = join(docsDirectory, '.vitepress', 'dist')
const artifactDirectory = process.env.DEMO_SMOKE_ARTIFACTS
const expectedSyntheticDigest = 'c9552b05'

const contentTypes = {
  '.css': 'text/css; charset=utf-8',
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.png': 'image/png',
  '.wasm': 'application/wasm',
  '.webmanifest': 'application/manifest+json; charset=utf-8',
}

function contentType(path) {
  return contentTypes[extname(path)] ?? 'application/octet-stream'
}

function artifactPath(requestPath) {
  if (!requestPath.startsWith('/frei0r/')) return undefined
  const relativePath = requestPath.slice('/frei0r/'.length) || 'index.html'
  const candidate = normalize(join(outputDirectory, relativePath.endsWith('/') ? `${relativePath}index.html` : relativePath))
  return candidate.startsWith(`${outputDirectory}${sep}`) || candidate === outputDirectory ? candidate : undefined
}

async function startServer() {
  const server = createServer(async (request, response) => {
    const path = artifactPath(new URL(request.url ?? '/', 'http://localhost').pathname)
    if (!path) {
      response.writeHead(404).end()
      return
    }
    try {
      const contents = await readFile(path)
      response.writeHead(200, {
        'cache-control': 'no-cache',
        'content-type': contentType(path),
      }).end(contents)
    } catch {
      response.writeHead(404).end()
    }
  })
  await new Promise((resolveServer, reject) => {
    server.once('error', reject)
    server.listen(0, '127.0.0.1', resolveServer)
  })
  const address = server.address()
  assert(address && typeof address !== 'string', 'The smoke server did not receive a local port.')
  return {
    origin: `http://127.0.0.1:${address.port}`,
    close: () => new Promise((resolveClose, reject) => server.close((error) => error ? reject(error) : resolveClose())),
  }
}

async function installSyntheticCamera(page, outcome = 'granted') {
  await page.addInitScript((requestedOutcome) => {
    const digest = (bytes) => {
      let value = 2166136261
      for (const byte of bytes) value = Math.imul(value ^ byte, 16777619)
      return (value >>> 0).toString(16).padStart(8, '0')
    }
    const metrics = {
      activeCallbacks: new Map(),
      constraints: [],
      digests: [],
      inputDigest: '',
      maxCallbacks: 0,
      presentations: 0,
      requests: 0,
      stoppedTracks: 0,
      streams: 0,
      uniqueOutputImages: 0,
      outputImages: new WeakSet(),
    }
    const videoPrototype = HTMLVideoElement.prototype
    const originalRequest = videoPrototype.requestVideoFrameCallback
    const originalCancel = videoPrototype.cancelVideoFrameCallback
    let nextCallback = 0
    let nextTimestamp = 0
    Object.defineProperty(videoPrototype, 'requestVideoFrameCallback', {
      configurable: true,
      value(callback) {
        const handle = ++nextCallback
        const animationHandle = requestAnimationFrame(() => {
          metrics.activeCallbacks.delete(handle)
          callback(nextTimestamp += 1000 / 30, {})
        })
        metrics.activeCallbacks.set(handle, animationHandle)
        metrics.maxCallbacks = Math.max(metrics.maxCallbacks, metrics.activeCallbacks.size)
        return handle
      },
    })
    Object.defineProperty(videoPrototype, 'cancelVideoFrameCallback', {
      configurable: true,
      value(handle) {
        const animationHandle = metrics.activeCallbacks.get(handle)
        if (animationHandle !== undefined) cancelAnimationFrame(animationHandle)
        metrics.activeCallbacks.delete(handle)
      },
    })
    const originalPutImageData = CanvasRenderingContext2D.prototype.putImageData
    CanvasRenderingContext2D.prototype.putImageData = function (imageData, ...args) {
      if (this.canvas.classList.contains('processed-frame')) {
        ++metrics.presentations
        if (!metrics.outputImages.has(imageData)) {
          metrics.outputImages.add(imageData)
          ++metrics.uniqueOutputImages
        }
        metrics.digests.push(digest(imageData.data))
      }
      return originalPutImageData.call(this, imageData, ...args)
    }
    const getUserMedia = async (constraints) => {
      ++metrics.requests
      metrics.constraints.push(constraints)
      if (requestedOutcome === 'denied') throw new DOMException('Denied by deterministic smoke coverage.', 'NotAllowedError')
      if (requestedOutcome === 'missing-media') throw new DOMException('No deterministic camera is available.', 'NotFoundError')

      const canvas = document.createElement('canvas')
      canvas.width = 160
      canvas.height = 120
      const context = canvas.getContext('2d', { willReadFrequently: true })
      const frame = context.createImageData(canvas.width, canvas.height)
      for (let y = 0; y < canvas.height; ++y) {
        for (let x = 0; x < canvas.width; ++x) {
          const index = (y * canvas.width + x) * 4
          frame.data[index] = (x * 5 + y * 3) & 255
          frame.data[index + 1] = (x * 7 + y * 11) & 255
          frame.data[index + 2] = (x * 13 + y * 17) & 255
          frame.data[index + 3] = 255
        }
      }
      context.putImageData(frame, 0, 0)
      metrics.inputDigest = digest(frame.data)
      const stream = canvas.captureStream(30)
      ++metrics.streams
      for (const track of stream.getTracks()) {
        const stop = track.stop.bind(track)
        track.stop = () => {
          ++metrics.stoppedTracks
          stop()
        }
      }
      return stream
    }
    Object.defineProperty(navigator.mediaDevices, 'getUserMedia', { configurable: true, value: getUserMedia })
    window.__frei0rDemoSmoke = {
      snapshot: () => ({
        activeCallbacks: metrics.activeCallbacks.size,
        constraints: metrics.constraints,
        digests: metrics.digests.slice(),
        inputDigest: metrics.inputDigest,
        maxCallbacks: metrics.maxCallbacks,
        presentations: metrics.presentations,
        requests: metrics.requests,
        stoppedTracks: metrics.stoppedTracks,
        streams: metrics.streams,
        uniqueOutputImages: metrics.uniqueOutputImages,
      }),
      restore: () => {
        Object.defineProperty(videoPrototype, 'requestVideoFrameCallback', { configurable: true, value: originalRequest })
        Object.defineProperty(videoPrototype, 'cancelVideoFrameCallback', { configurable: true, value: originalCancel })
        CanvasRenderingContext2D.prototype.putImageData = originalPutImageData
      },
    }
  }, outcome)
}

async function snapshot(page) {
  return page.evaluate(() => window.__frei0rDemoSmoke.snapshot())
}

async function waitForPresentations(page, previous, count = 3) {
  await page.waitForFunction(({ previousCount, requiredCount }) =>
    window.__frei0rDemoSmoke.snapshot().presentations >= previousCount + requiredCount,
  { previousCount: previous, requiredCount: count })
}

async function startCamera(page) {
  await page.getByRole('button', { name: 'Start camera' }).click()
  await page.locator('[data-stage="running"]').waitFor()
  await waitForPresentations(page, 0)
}

function attachExceptionCollection(page) {
  const exceptions = []
  page.on('pageerror', (error) => exceptions.push(error.message))
  return () => assert.deepEqual(exceptions, [], `Page exceptions: ${exceptions.join('; ')}`)
}

async function runGrantedScenario(browser, origin) {
  const context = await browser.newContext({ serviceWorkers: 'allow', viewport: { width: 1280, height: 800 } })
  const page = await context.newPage()
  const assertNoExceptions = attachExceptionCollection(page)
  await installSyntheticCamera(page)
  await page.goto(`${origin}/frei0r/demo/`, { waitUntil: 'networkidle' })
  const wasmResponse = await page.request.get(`${origin}/frei0r/demo/runtime/${await page.evaluate(async () => {
    const manifest = await fetch('./runtime/runtime-manifest.json').then((response) => response.json())
    return manifest.wasm
  })}`)
  assert.equal(wasmResponse.headers()['content-type'], 'application/wasm', 'The local smoke server must serve Wasm with its HTTP MIME type.')
  await startCamera(page)

  const initial = await snapshot(page)
  assert.equal(initial.inputDigest, expectedSyntheticDigest, 'Synthetic RGBA input changed unexpectedly.')
  assert.deepEqual(initial.constraints, [{ audio: false, video: true }], 'The demo must request video-only camera access.')
  assert.ok(initial.maxCallbacks <= 1, 'The scheduler queued more than one video callback.')

  const catalog = await page.locator('.filter-rail button').evaluateAll((buttons) => buttons.map((button) => button.id))
  assert.ok(catalog.length >= 12, 'The browser runtime did not expose the curated filter catalog.')
  const digests = {}
  for (const id of catalog) {
    const before = await snapshot(page)
    await page.locator(`#${id}`).click()
    await waitForPresentations(page, before.presentations, 8)
    const after = await snapshot(page)
    assert.ok(after.presentations >= before.presentations + 8, `${id} did not present multiple frames.`)
    digests[id] = after.digests.at(-1)
    assert.match(digests[id], /^[0-9a-f]{8}$/, `${id} did not produce a deterministic frame digest.`)
  }
  const allocationCheck = await snapshot(page)
  assert.ok(allocationCheck.uniqueOutputImages <= catalog.length + 2,
    'The renderer retained more output images than the selected filter configurations.')

  await page.locator('#filter-0').click()
  await page.getByRole('button', { name: /Parameters/ }).click()
  const parameter = page.locator('#parameter-content input[type="range"]').first()
  await parameter.waitFor()
  const beforeParameter = await snapshot(page)
  await parameter.evaluate((input) => {
    input.value = '1'
    input.dispatchEvent(new Event('input', { bubbles: true }))
  })
  await waitForPresentations(page, beforeParameter.presentations)
  const afterParameter = await snapshot(page)
  assert.notEqual(afterParameter.digests.at(-1), beforeParameter.digests.at(-1), 'Changing a parameter did not change the rendered frame.')

  await page.getByRole('button', { name: 'Stop camera' }).click()
  await page.locator('[data-stage="stopped"]').waitFor()
  const stopped = await snapshot(page)
  assert.equal(stopped.activeCallbacks, 0, 'Stopping the camera left a scheduled frame callback.')
  assert.equal(stopped.stoppedTracks, 1, 'Stopping the camera did not release its track.')

  await page.getByRole('button', { name: 'Restart camera' }).click()
  await page.locator('[data-stage="running"]').waitFor()
  await waitForPresentations(page, stopped.presentations)
  await page.evaluate(() => window.dispatchEvent(new PageTransitionEvent('pagehide')))
  await page.locator('[data-stage="stopped"]').waitFor()
  const pageHidden = await snapshot(page)
  assert.equal(pageHidden.activeCallbacks, 0, 'Page hide left a scheduled frame callback.')
  assert.equal(pageHidden.stoppedTracks, 2, 'Page hide did not release the restarted track.')
  assert.equal(pageHidden.streams, 2, 'The lifecycle scenario did not start and restart exactly once.')

  await page.reload({ waitUntil: 'networkidle' })
  await page.waitForFunction(() => navigator.serviceWorker.controller !== null)
  const serviceWorker = await page.evaluate(async () => ({
    rootRegistration: await navigator.serviceWorker.getRegistration('/frei0r/'),
    scope: (await navigator.serviceWorker.getRegistration())?.scope,
  }))
  assert.equal(serviceWorker.rootRegistration, undefined, 'The demo worker must not control documentation routes.')
  assert.equal(serviceWorker.scope, `${origin}/frei0r/demo/`, 'The demo worker scope is broader than the demo route.')
  await context.setOffline(true)
  await page.reload({ waitUntil: 'domcontentloaded' })
  await page.getByRole('heading', { name: 'Live filters, running locally' }).waitFor()
  await context.setOffline(false)
  assertNoExceptions()
  await context.close()
  return { catalog, digests, lifecycle: pageHidden }
}

async function runCameraFailureScenario(browser, origin, outcome, heading) {
  const context = await browser.newContext({ serviceWorkers: 'block' })
  const page = await context.newPage()
  const assertNoExceptions = attachExceptionCollection(page)
  await installSyntheticCamera(page, outcome)
  await page.goto(`${origin}/frei0r/demo/`, { waitUntil: 'networkidle' })
  await page.getByRole('button', { name: 'Start camera' }).click()
  await page.getByRole('heading', { name: heading }).waitFor()
  const result = await snapshot(page)
  assert.equal(result.requests, 1, `${outcome} did not request the camera exactly once.`)
  assert.equal(result.streams, 0, `${outcome} created a stream.`)
  assert.equal(result.activeCallbacks, 0, `${outcome} left a scheduled frame callback.`)
  assertNoExceptions()
  await context.close()
  return result
}

async function writeEvidence(evidence) {
  if (!artifactDirectory) return
  await mkdir(artifactDirectory, { recursive: true })
  await writeFile(join(artifactDirectory, 'demo-browser-smoke.json'), `${JSON.stringify(evidence, null, 2)}\n`)
}

const server = await startServer()
const browser = await chromium.launch({
  headless: true,
  ...(process.env.DEMO_SMOKE_CHROMIUM_EXECUTABLE
    ? { executablePath: process.env.DEMO_SMOKE_CHROMIUM_EXECUTABLE }
    : {}),
  args: [
    '--use-fake-device-for-media-stream',
    '--use-fake-ui-for-media-stream',
    '--use-angle=swiftshader',
    '--enable-unsafe-swiftshader',
  ],
})

try {
  const granted = await runGrantedScenario(browser, server.origin)
  const denied = await runCameraFailureScenario(browser, server.origin, 'denied', 'Camera permission was not granted')
  const missingMedia = await runCameraFailureScenario(browser, server.origin, 'missing-media', 'No usable camera is available')
  const evidence = { granted, denied, missingMedia }
  await writeEvidence(evidence)
  console.log(`Browser smoke passed: filters=${granted.catalog.length} minimum-frames-per-filter=8 lifecycle-streams=${granted.lifecycle.streams} denied=${denied.requests} missing-media=${missingMedia.requests}`)
} finally {
  await browser.close()
  await server.close()
}
