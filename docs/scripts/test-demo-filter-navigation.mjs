import { mkdtemp, readFile, rm, writeFile } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { spawn } from 'node:child_process'
import ts from 'typescript'

const sourcePath = new URL('../demo/src/filter-navigation.ts', import.meta.url)
const testPath = new URL('../demo/test/filter-navigation.test.mjs', import.meta.url)
const temporaryDirectory = await mkdtemp(join(tmpdir(), 'frei0r-demo-filter-navigation-'))
const modulePath = join(temporaryDirectory, 'filter-navigation.mjs')

try {
  const source = await readFile(sourcePath, 'utf8')
  const { outputText } = ts.transpileModule(source, {
    compilerOptions: { module: ts.ModuleKind.ESNext, target: ts.ScriptTarget.ES2022 },
    fileName: fileURLToPath(sourcePath)
  })
  await writeFile(modulePath, outputText)
  const child = spawn(process.execPath, ['--test', fileURLToPath(testPath)], {
    env: { ...process.env, FILTER_NAVIGATION_MODULE: new URL(`file://${modulePath}`).href },
    stdio: 'inherit'
  })
  await new Promise((resolve, reject) => {
    child.once('error', reject)
    child.once('exit', (code) => code === 0 ? resolve() : reject(new Error(`Filter navigation tests exited with ${code}.`)))
  })
} finally {
  await rm(temporaryDirectory, { force: true, recursive: true })
}
