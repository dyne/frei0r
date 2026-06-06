import { spawnSync } from 'node:child_process'
import { fileURLToPath } from 'node:url'
import { dirname, resolve } from 'node:path'

const docsDirectory = resolve(dirname(fileURLToPath(import.meta.url)), '..')
const repositoryRoot = resolve(docsDirectory, '..')

const result = spawnSync('doxygen', ['docs/Doxyfile'], {
  cwd: repositoryRoot,
  shell: process.platform === 'win32',
  stdio: 'inherit'
})

if (result.error || result.status !== 0) {
  console.error('Doxygen is required to build the complete website.')
  if (result.error) {
    console.error(result.error.message)
  }
  process.exit(1)
}

process.exit(0)
