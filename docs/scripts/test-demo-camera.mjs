import { mkdtemp, readFile, rm, writeFile } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'
import { spawn } from 'node:child_process'
import ts from 'typescript'

const cameraSource = new URL('../demo/src/camera.ts', import.meta.url)
const cameraTest = new URL('../demo/test/camera.test.mjs', import.meta.url)
const temporaryDirectory = await mkdtemp(join(tmpdir(), 'frei0r-demo-camera-'))
const modulePath = join(temporaryDirectory, 'camera.mjs')

try {
  const source = await readFile(cameraSource, 'utf8')
  const { outputText } = ts.transpileModule(source, {
    compilerOptions: {
      module: ts.ModuleKind.ESNext,
      target: ts.ScriptTarget.ES2022
    },
    fileName: fileURLToPath(cameraSource)
  })
  await writeFile(modulePath, outputText)

  const child = spawn(process.execPath, ['--test', fileURLToPath(cameraTest)], {
    env: { ...process.env, CAMERA_MODULE: new URL(`file://${modulePath}`).href },
    stdio: 'inherit'
  })
  await new Promise((resolve, reject) => {
    child.once('error', reject)
    child.once('exit', (code) => code === 0 ? resolve() : reject(new Error(`Camera tests exited with ${code}.`)))
  })
} finally {
  await rm(temporaryDirectory, { force: true, recursive: true })
}
