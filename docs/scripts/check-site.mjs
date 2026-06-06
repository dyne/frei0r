import { existsSync, readdirSync, readFileSync } from 'node:fs'
import { dirname, extname, join, relative, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'

const docsDirectory = resolve(dirname(fileURLToPath(import.meta.url)), '..')
const outputDirectory = join(docsDirectory, '.vitepress', 'dist')

if (!existsSync(join(outputDirectory, 'index.html'))) {
  throw new Error('Build the website before running the checker.')
}

const htmlFiles = []

function collectHtml(directory) {
  for (const entry of readdirSync(directory, { withFileTypes: true })) {
    const path = join(directory, entry.name)
    if (entry.isDirectory()) {
      if (relative(outputDirectory, path) === 'codedoc') {
        continue
      }
      collectHtml(path)
    } else if (extname(entry.name) === '.html') {
      htmlFiles.push(path)
    }
  }
}

function localCandidates(sourceFile, value) {
  const withoutFragment = value.split('#', 1)[0].split('?', 1)[0]
  if (!withoutFragment || withoutFragment.startsWith('//')) {
    return []
  }

  const base = withoutFragment.startsWith('/')
    ? outputDirectory
    : dirname(sourceFile)
  const target = resolve(base, withoutFragment.replace(/^\/+/, ''))

  return [target, `${target}.html`, join(target, 'index.html')]
}

collectHtml(outputDirectory)

const missing = []
const attributePattern = /\b(?:href|src)=["']([^"'<>]+)["']/g

for (const htmlFile of htmlFiles) {
  const html = readFileSync(htmlFile, 'utf8')
  for (const match of html.matchAll(attributePattern)) {
    const value = match[1]
    if (/^(?:[a-z]+:|#)/i.test(value)) {
      continue
    }

    const candidates = localCandidates(htmlFile, decodeURI(value))
    if (candidates.length > 0 && !candidates.some(existsSync)) {
      missing.push(`${relative(outputDirectory, htmlFile)} -> ${value}`)
    }
  }
}

const retainedPaths = [
  'index.html',
  'pics/fla_name_lb.webp',
  'pics/frei0r.png',
  'frei0r-all.webm',
  'codedoc/html/index.html',
  'codedoc/html/frei0r_8h.html'
]

for (const path of retainedPaths) {
  if (!existsSync(join(outputDirectory, path))) {
    missing.push(`retained path missing: ${path}`)
  }
}

if (missing.length > 0) {
  console.error(missing.join('\n'))
  process.exit(1)
}

console.log(`Checked ${htmlFiles.length} HTML files and ${retainedPaths.length} retained paths.`)
