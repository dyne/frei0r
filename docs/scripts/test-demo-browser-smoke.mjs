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
      inputFrameAllocations: 0,
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
    const originalGetImageData = CanvasRenderingContext2D.prototype.getImageData
    CanvasRenderingContext2D.prototype.getImageData = function (...args) {
      ++metrics.inputFrameAllocations
      return originalGetImageData.call(this, ...args)
    }
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
      // Publish after capture starts so the synthetic video receives a real frame.
      context.putImageData(frame, 0, 0)
      const paint = setInterval(() => context.putImageData(frame, 0, 0), 1000 / 30)
      ++metrics.streams
      for (const track of stream.getTracks()) {
        const stop = track.stop.bind(track)
        track.stop = () => {
          clearInterval(paint)
          ++metrics.stoppedTracks
          stop()
        }
      }
      return stream
    }
    Object.defineProperty(navigator.mediaDevices, 'getUserMedia', { configurable: true, value: getUserMedia })
    window.__frei0rDemoSmoke = {
      sampleVideoDigest: () => {
        const video = document.querySelector('video')
        const sample = document.createElement('canvas')
        sample.width = video.videoWidth
        sample.height = video.videoHeight
        const context = sample.getContext('2d', { willReadFrequently: true })
        context.drawImage(video, 0, 0)
        return digest(originalGetImageData.call(context, 0, 0, sample.width, sample.height).data)
      },
      snapshot: () => ({
        activeCallbacks: metrics.activeCallbacks.size,
        constraints: metrics.constraints,
        digests: metrics.digests.slice(),
        inputDigest: metrics.inputDigest,
        inputFrameAllocations: metrics.inputFrameAllocations,
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
        CanvasRenderingContext2D.prototype.getImageData = originalGetImageData
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
  await page.getByRole('button', { name: 'Controls', exact: true }).click()

  const initial = await snapshot(page)
  assert.equal(initial.inputDigest, expectedSyntheticDigest, 'Synthetic RGBA input changed unexpectedly.')
  const presentedInputDigest = await page.evaluate(() => window.__frei0rDemoSmoke.sampleVideoDigest())
  assert.equal(presentedInputDigest, expectedSyntheticDigest, 'The synthetic camera must deliver its colored test frame.')
  assert.equal(initial.digests.at(-1), presentedInputDigest,
    'The zero-conversion capture path changed the top-to-bottom RGBA input.')
  assert.deepEqual(initial.constraints, [{ audio: false, video: true }], 'The demo must request video-only camera access.')
  assert.ok(initial.maxCallbacks <= 1, 'The scheduler queued more than one video callback.')
  assert.equal(initial.inputFrameAllocations, 0,
    'The browser did not use the zero-churn capture path for camera frames.')

  const catalog = await page.locator('.filter-rail button').evaluateAll((buttons) => buttons.map((button) => button.id))
  assert.ok(catalog.length >= 80, 'The browser runtime did not expose the expanded filter catalog.')
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
  assert.equal(allocationCheck.inputFrameAllocations, 0,
    'The primary capture path allocated Canvas ImageData while rendering.')

  await page.getByRole('button', { name: 'Close controls' }).click()
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

async function runGestureScenario(browser, origin, mobile, landscape = false) {
  const viewport = mobile
    ? landscape ? { width: 844, height: 390 } : { width: 390, height: 844 }
    : { width: 1440, height: 900 }
  const context = await browser.newContext({ viewport, hasTouch: mobile, isMobile: mobile, serviceWorkers: 'block' })
  const page = await context.newPage()
  const assertNoExceptions = attachExceptionCollection(page)
  await installSyntheticCamera(page)
  await page.goto(`${origin}/frei0r/demo/`, { waitUntil: 'networkidle' })
  await startCamera(page)
  const surface = page.locator('.gesture-surface')
  const frame = await page.locator('.stage-frame').boundingBox()
  assert(frame)
  if (mobile) {
    assert.equal(frame.width, viewport.width, 'Mobile preview must fill the viewport width.')
    assert.equal(frame.height, viewport.height, 'Mobile preview must fill the viewport height.')
    assert.equal(frame.y, 0, 'Mobile preview must start at the top of the viewport.')
  } else {
    assert.ok(frame.width > viewport.width * 0.9 && frame.height > viewport.height * 0.8,
      'Desktop preview must occupy most of the viewport.')
  }
  const cdp = mobile ? await context.newCDPSession(page) : undefined
  async function swipe(dx, dy, during) {
    const x = frame.x + frame.width / 2, y = frame.y + frame.height / 2
    if (cdp) {
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [{ x, y, id: 0 }] })
      for (let step = 1; step <= 8; ++step) {
        await cdp.send('Input.dispatchTouchEvent', { type: 'touchMove', touchPoints: [{ x: x + dx * step / 8, y: y + dy * step / 8, id: 0 }] })
      }
      if (during) await during()
      await cdp.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] })
    } else {
      await page.mouse.move(x, y)
      await page.mouse.down()
      await page.mouse.move(x + dx, y + dy, { steps: 8 })
      if (during) await during()
      await page.mouse.up()
    }
  }
  const initialName = await page.locator('.filter-name').textContent()
  await swipe(-100, 0)
  await page.waitForFunction((name) => document.querySelector('.filter-name')?.textContent !== name, initialName)
  await page.waitForFunction(() => document.querySelector('.dominant-parameter')?.textContent === 'No adjustable parameters')
  await swipe(100, 0)
  await page.waitForFunction((name) => document.querySelector('.filter-name')?.textContent === name, initialName)
  await page.locator('.dominant-parameter output').waitFor()
  const initialValue = Number(await page.locator('.dominant-parameter output').textContent())
  await swipe(0, -80, async () => {
    assert.ok(Number(await page.locator('.dominant-parameter output').textContent()) > initialValue,
      'Vertical adjustment must update continuously before the gesture ends.')
  })
  const increased = Number(await page.locator('.dominant-parameter output').textContent())
  await swipe(0, 80)
  assert.ok(Number(await page.locator('.dominant-parameter output').textContent()) < increased)
  assert.equal(await page.locator('.filter-name').textContent(), initialName, 'A vertical swipe changed filters.')
  await swipe(4, 4)
  assert.equal(await page.locator('.filter-name').textContent(), initialName, 'A tap-sized movement changed filters.')
  await surface.focus()
  assert.equal(await surface.evaluate((element) => getComputedStyle(element).backgroundColor),
    'rgba(0, 0, 0, 0)', 'The gesture surface must not obscure the video on hover or focus.')
  await page.keyboard.press('ArrowUp')
  assert.ok(Number(await page.locator('.dominant-parameter output').textContent()) > initialValue)
  await page.waitForFunction(() => Number(document.querySelector('.fps')?.textContent?.split(' ')[0]) > 0)
  assert.equal(await page.evaluate(() => document.documentElement.scrollWidth > innerWidth), false, 'Page has horizontal overflow.')
  if (artifactDirectory) {
    await mkdir(artifactDirectory, { recursive: true })
    await page.screenshot({ path: join(artifactDirectory, mobile
      ? landscape ? 'mobile-landscape-camera.png' : 'mobile-camera.png'
      : 'desktop-camera.png') })
  }
  await page.getByRole('button', { name: 'Controls', exact: true }).click()
  await page.locator('.parameter-toggle').click()
  const parameters = page.locator('.parameter-content input')
  assert.ok(await parameters.count() > 0, 'Full parameter controls are unavailable.')
  await page.getByRole('button', { name: 'Close controls' }).click()
  await page.getByRole('button', { name: 'Stop camera' }).click()
  assertNoExceptions()
  await context.close()
  return { viewport, frame, input: mobile ? 'Chromium synthesized touch' : 'Chromium mouse and keyboard' }
}

async function runPlaybackFailureScenario(browser, origin) {
  const context = await browser.newContext({ serviceWorkers: 'block' })
  const page = await context.newPage()
  const assertNoExceptions = attachExceptionCollection(page)
  await installSyntheticCamera(page)
  await page.addInitScript(() => {
    const play = HTMLVideoElement.prototype.play
    let rejected = false
    HTMLVideoElement.prototype.play = function () {
      if (this.srcObject && !rejected) {
        rejected = true
        return Promise.reject(new Error('Synthetic playback failure'))
      }
      return play.call(this)
    }
  })
  await page.goto(`${origin}/frei0r/demo/`, { waitUntil: 'networkidle' })
  await page.getByRole('button', { name: 'Start camera' }).click()
  await page.getByRole('heading', { name: 'Filter processing paused' }).waitFor()
  assert.ok((await page.locator('.stage-overlay').textContent()).includes('Synthetic playback failure'))
  await page.getByRole('button', { name: 'Controls', exact: true }).click()
  const panel = await page.locator('.action-dock').boundingBox()
  assert(panel && panel.y >= 0 && panel.y + panel.height <= 720,
    'Filter controls must remain inside the viewport during playback failure.')
  await page.getByRole('button', { name: 'Close controls' }).click()
  await page.getByRole('button', { name: 'Retry processing' }).click()
  await page.locator('[data-stage="running"]').waitFor()
  await waitForPresentations(page, 0)
  await page.getByRole('button', { name: 'Stop camera' }).click()
  assert.equal((await snapshot(page)).stoppedTracks, 1)
  assertNoExceptions()
  await context.close()
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
  const desktop = await runGestureScenario(browser, server.origin, false)
  const mobile = await runGestureScenario(browser, server.origin, true)
  const mobileLandscape = await runGestureScenario(browser, server.origin, true, true)
  await runPlaybackFailureScenario(browser, server.origin)
  const evidence = { granted, denied, missingMedia, desktop, mobile, mobileLandscape }
  await writeEvidence(evidence)
  console.log(`Browser smoke passed: filters=${granted.catalog.length} minimum-frames-per-filter=8 lifecycle-streams=${granted.lifecycle.streams} denied=${denied.requests} missing-media=${missingMedia.requests}`)
} finally {
  await browser.close()
  await server.close()
}
