import { mkdtemp, readFile, rm, writeFile } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { spawn } from 'node:child_process'
import ts from 'typescript'

const source = new URL('../demo/src/stage-state.ts', import.meta.url)
const testFile = new URL('../demo/test/stage-state.test.mjs', import.meta.url)
const temporaryDirectory = await mkdtemp(join(tmpdir(), 'frei0r-demo-stage-state-'))
const modulePath = join(temporaryDirectory, 'stage-state.mjs')

try {
  const input = await readFile(source, 'utf8')
  const { outputText } = ts.transpileModule(input, {
    compilerOptions: { module: ts.ModuleKind.ESNext, target: ts.ScriptTarget.ES2022 },
    fileName: fileURLToPath(source)
  })
  await writeFile(modulePath, outputText)
  const child = spawn(process.execPath, ['--test', fileURLToPath(testFile)], {
    env: { ...process.env, STAGE_STATE_MODULE: new URL(`file://${modulePath}`).href },
    stdio: 'inherit'
  })
  await new Promise((resolve, reject) => {
    child.once('error', reject)
    child.once('exit', (code) => code === 0 ? resolve() : reject(new Error(`Stage-state tests exited with ${code}.`)))
  })
} finally {
  await rm(temporaryDirectory, { force: true, recursive: true })
}
