import assert from 'node:assert/strict'
import { readFileSync } from 'node:fs'
import vm from 'node:vm'

const source = readFileSync('.vitepress/dist/demo/service-worker.js', 'utf8')
const listeners = new Map()
const responses = new Map([['./index.html', new Response('offline shell')]])
let responded = false

const self = {
  registration: { scope: 'https://example.test/frei0r/demo/' },
  location: { origin: 'https://example.test' },
  clients: { claim: async () => undefined },
  skipWaiting: async () => undefined,
  addEventListener: (type, listener) => listeners.set(type, listener),
}
const caches = {
  open: async () => ({
    addAll: async () => undefined,
    put: async () => undefined,
  }),
  keys: async () => [],
  delete: async () => true,
  match: async (request) => responses.get(typeof request === 'string' ? request : undefined),
}

vm.runInNewContext(source, {
  URL,
  Promise,
  caches,
  fetch: async () => { throw new Error('offline') },
  self,
})

await new Promise((resolve, reject) => listeners.get('install')({ waitUntil: (promise) => promise.then(resolve, reject) }))

listeners.get('fetch')({
  request: {
    url: 'https://example.test/frei0r/guide/',
    method: 'GET',
    mode: 'navigate',
  },
  respondWith: () => { responded = true },
})
assert.equal(responded, false, 'the demo service worker must not intercept documentation routes')

let offlineResponse
listeners.get('fetch')({
  request: {
    url: 'https://example.test/frei0r/demo/',
    method: 'GET',
    mode: 'navigate',
  },
  respondWith: (promise) => { offlineResponse = promise },
})
assert.equal((await offlineResponse).status, 200, 'offline navigation should receive the cached shell')
console.log('Demo service worker keeps documentation outside its scope and serves the cached shell offline.')
