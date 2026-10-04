import { existsSync, readFileSync } from 'node:fs'
import { dirname, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'

const docsDirectory = resolve(dirname(fileURLToPath(import.meta.url)), '..')
const documents = ['README.md', 'DEMO.md']
const missing = []

for (const document of documents) {
  const path = resolve(docsDirectory, document)
  const markdown = readFileSync(path, 'utf8')
  for (const match of markdown.matchAll(/\[[^\]]+\]\(([^)]+)\)/g)) {
    const target = match[1].split('#', 1)[0]
    if (!target || /^(?:[a-z]+:|\/|#)/i.test(target)) continue
    if (!existsSync(resolve(dirname(path), target))) {
      missing.push(`${document} -> ${target}`)
    }
  }
}

if (missing.length > 0) {
  throw new Error(`Broken documentation links:\n${missing.join('\n')}`)
}

console.log(`Checked relative links in ${documents.length} maintainer documents.`)
