import { mkdtemp, readFile, rm, writeFile } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { spawn } from 'node:child_process'
import ts from 'typescript'

const pipelineSource = new URL('../demo/src/frame-pipeline.ts', import.meta.url)
const pipelineTest = new URL('../demo/test/frame-pipeline.test.mjs', import.meta.url)
const temporaryDirectory = await mkdtemp(join(tmpdir(), 'frei0r-demo-frame-pipeline-'))
const modulePath = join(temporaryDirectory, 'frame-pipeline.mjs')

try {
  const source = await readFile(pipelineSource, 'utf8')
  const { outputText } = ts.transpileModule(source, {
    compilerOptions: { module: ts.ModuleKind.ESNext, target: ts.ScriptTarget.ES2022 },
    fileName: fileURLToPath(pipelineSource)
  })
  await writeFile(modulePath, outputText)
  const child = spawn(process.execPath, ['--test', fileURLToPath(pipelineTest)], {
    env: { ...process.env, FRAME_PIPELINE_MODULE: new URL(`file://${modulePath}`).href },
    stdio: 'inherit'
  })
  await new Promise((resolve, reject) => {
    child.once('error', reject)
    child.once('exit', (code) => code === 0 ? resolve() : reject(new Error(`Frame pipeline tests exited with ${code}.`)))
  })
} finally {
  await rm(temporaryDirectory, { force: true, recursive: true })
}
