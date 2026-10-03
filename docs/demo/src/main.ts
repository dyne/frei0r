import { mount } from 'svelte'
import App from './App.svelte'
import { initializeFrei0rDemoRuntime } from './runtime'
import { registerDemoServiceWorker } from './service-worker'

const target = document.getElementById('app')

if (!target) {
  throw new Error('The demo application mount point is missing.')
}

mount(App, { target })

void initializeFrei0rDemoRuntime().catch((error: unknown) => {
  console.error('Unable to initialize the frei0r browser runtime.', error)
})

void registerDemoServiceWorker().catch((error: unknown) => {
  console.error('Unable to register the frei0r demo service worker.', error)
})
