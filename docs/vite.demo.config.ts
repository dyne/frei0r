import { svelte } from '@sveltejs/vite-plugin-svelte'
import { defineConfig } from 'vite'
import { dirname, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'

const docsDirectory = dirname(fileURLToPath(import.meta.url))
const demoDirectory = resolve(docsDirectory, 'demo')

function demoBase(basePath = process.env.BASE_PATH ?? '/frei0r/') {
  const normalizedBase = basePath === '/'
    ? '/'
    : `/${basePath.replace(/^\/+|\/+$/g, '')}/`

  return `${normalizedBase}demo/`
}

export default defineConfig({
  appType: 'spa',
  base: demoBase(),
  plugins: [svelte({ configFile: resolve(docsDirectory, 'svelte.config.js') })],
  root: demoDirectory,
  build: {
    emptyOutDir: false,
    manifest: true,
    outDir: resolve(docsDirectory, '.vitepress', 'dist', 'demo')
  }
})
