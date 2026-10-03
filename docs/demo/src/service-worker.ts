export async function registerDemoServiceWorker(): Promise<void> {
  if (!('serviceWorker' in navigator)) {
    return
  }

  const baseUrl = import.meta.env.BASE_URL
  const scope = new URL(baseUrl, window.location.origin).href

  await navigator.serviceWorker.register(`${baseUrl}service-worker.js`, { scope })
}
