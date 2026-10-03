import assert from 'node:assert/strict'
import AxeBuilder from '@axe-core/playwright'
import { chromium } from 'playwright-core'

const url = process.env.DEMO_A11Y_URL ?? 'http://127.0.0.1:5175/frei0r/demo/?stage=running&controls=maximum&feedback=offline'
const browser = await chromium.launch({
  headless: true,
  ...(process.env.DEMO_A11Y_CHROMIUM_EXECUTABLE
    ? { executablePath: process.env.DEMO_A11Y_CHROMIUM_EXECUTABLE }
    : {})
})

try {
  const context = await browser.newContext({ viewport: { width: 390, height: 844 } })
  const page = await context.newPage()
  await page.goto(url, { waitUntil: 'networkidle' })
  const results = await new AxeBuilder({ page })
    .withTags(['wcag2a', 'wcag2aa', 'wcag21aa', 'wcag22aa'])
    .analyze()
  const severities = Object.fromEntries(['critical', 'serious', 'moderate', 'minor']
    .map((impact) => [impact, results.violations.filter((violation) => violation.impact === impact).length]))

  console.log(`Axe accessibility audit: violations=${results.violations.length} critical=${severities.critical} serious=${severities.serious} moderate=${severities.moderate} minor=${severities.minor}`)
  for (const violation of results.violations) {
    console.error(`${violation.impact ?? 'unknown'} ${violation.id}: ${violation.nodes.map((node) => node.target.join(' ')).join(', ')}`)
  }
  assert.equal(severities.critical, 0, 'Axe found critical accessibility violations.')
  assert.equal(severities.serious, 0, 'Axe found serious accessibility violations.')
  await context.close()
} finally {
  await browser.close()
}
