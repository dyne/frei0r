<script lang="ts">
  import { onDestroy, onMount, tick } from 'svelte'
  import { CameraService, type CameraSnapshot } from './camera'
  import { FilterParameters, type FilterParameter } from './filter-parameters'
  import { FramePipeline } from './frame-pipeline'
  import { FrameScheduler } from './frame-scheduler'
  import { initializeFrei0rDemoRuntime } from './runtime'

  const camera = new CameraService()
  let cameraState: CameraSnapshot = camera.snapshot
  let sourceVideo: HTMLVideoElement
  let captureCanvas: HTMLCanvasElement
  let outputCanvas: HTMLCanvasElement
  let scheduler: FrameScheduler | undefined
  let parameters: FilterParameters | undefined
  let filterParameters: readonly FilterParameter[] = []
  let schedulerStatus = 'Preparing the local WebAssembly runtime.'
  let unsubscribeScheduler = () => {}
  let unsubscribeParameters = () => {}
  let unsubscribeConfiguration = () => {}
  const unsubscribe = camera.subscribe((snapshot) => {
    cameraState = snapshot
    if (snapshot.status !== 'active') scheduler?.stop()
  })

  $: if (sourceVideo) sourceVideo.srcObject = (cameraState.stream as MediaStream | undefined) ?? null

  onMount(() => {
    void initializeEngine()
    return () => {
      scheduler?.stop()
      unsubscribeScheduler()
      unsubscribeParameters()
      unsubscribeConfiguration()
    }
  })

  async function initializeEngine() {
    try {
      const runtime = await initializeFrei0rDemoRuntime()
      const pipeline = new FramePipeline(runtime, { captureCanvas, outputCanvas })
      scheduler = new FrameScheduler(pipeline, sourceVideo)
      parameters = new FilterParameters(runtime)
      scheduler.selectFilter(0)
      unsubscribeScheduler = scheduler.subscribe((snapshot) => {
        schedulerStatus = snapshot.status
      })
      unsubscribeParameters = parameters.subscribe((nextParameters) => {
        filterParameters = nextParameters
      })
      unsubscribeConfiguration = scheduler.subscribeConfiguration(() => {
        parameters?.refresh()
      })
      if (cameraState.status === 'active') scheduler.start()
    } catch (error) {
      schedulerStatus = error instanceof Error ? error.message : 'The local filter runtime could not start.'
    }
  }

  async function toggleCamera() {
    if (cameraState.status === 'active' || cameraState.status === 'starting') {
      scheduler?.stop()
      camera.stop()
      return
    }
    const result = await camera.start()
    if (result.ok) {
      await tick()
      scheduler?.start()
    }
  }

  function setBoolean(parameter: FilterParameter, event: Event) {
    parameters?.set(parameter.index, (event.currentTarget as HTMLInputElement).checked)
  }

  function setNumber(parameter: FilterParameter, event: Event) {
    parameters?.set(parameter.index, Number((event.currentTarget as HTMLInputElement).value))
  }

  function colorValue([red, green, blue]: readonly [number, number, number]) {
    return `#${[red, green, blue].map((component) => Math.round(component * 255).toString(16).padStart(2, '0')).join('')}`
  }

  function setColor(parameter: FilterParameter, event: Event) {
    const value = (event.currentTarget as HTMLInputElement).value
    parameters?.set(parameter.index, [
      Number.parseInt(value.slice(1, 3), 16) / 255,
      Number.parseInt(value.slice(3, 5), 16) / 255,
      Number.parseInt(value.slice(5, 7), 16) / 255
    ])
  }

  function setPosition(parameter: FilterParameter, component: number, event: Event) {
    if (parameter.kind !== 'position') return
    const next = [...parameter.value] as [number, number]
    next[component] = Number((event.currentTarget as HTMLInputElement).value)
    parameters?.set(parameter.index, next)
  }

  onDestroy(() => {
    unsubscribe()
    camera.destroy()
  })
</script>

<svelte:head>
  <meta
    name="description"
    content="A local, in-browser frei0r video-filter demonstration."
  />
</svelte:head>

<main>
  <header>
    <p class="eyebrow">frei0r browser demo</p>
    <p class="status" aria-live="polite">
      {cameraState.error?.message ?? (cameraState.status === 'active' ? schedulerStatus : 'Camera is off.')}
    </p>
  </header>

  <section aria-labelledby="demo-title">
    <h1 id="demo-title">Live filters, running locally</h1>
    <p>
      This demo processes its camera and frei0r runtime in your browser. Video
      and audio are never uploaded.
    </p>
    <button type="button" on:click={toggleCamera} disabled={cameraState.status === 'starting'}>
      {cameraState.status === 'active' ? 'Stop camera' : cameraState.status === 'starting' ? 'Starting camera…' : 'Start camera'}
    </button>
    <video bind:this={sourceVideo} autoplay muted playsinline aria-label="Camera source" hidden></video>
    <canvas bind:this={captureCanvas} hidden></canvas>
    <canvas bind:this={outputCanvas} class="processed-frame" aria-label="Filtered camera preview" hidden={cameraState.status !== 'active'}></canvas>
  </section>

  {#if filterParameters.length > 0}
    <section aria-labelledby="parameter-title">
      <h2 id="parameter-title">Filter parameters</h2>
      {#each filterParameters as parameter (parameter.index)}
        <label title={parameter.explanation}>
          <span>{parameter.name}</span>
          {#if parameter.kind === 'boolean'}
            <input type="checkbox" checked={parameter.value} on:change={(event) => setBoolean(parameter, event)} />
          {:else if parameter.kind === 'number'}
            <input type="range" min="0" max="1" step="0.01" value={parameter.value} on:input={(event) => setNumber(parameter, event)} />
          {:else if parameter.kind === 'color'}
            <input type="color" value={colorValue(parameter.value)} on:input={(event) => setColor(parameter, event)} />
          {:else}
            <span class="position-control">
              {#each parameter.value as coordinate, component}
                <input aria-label={`${parameter.name} ${component === 0 ? 'horizontal' : 'vertical'}`} type="range" min="0" max="1" step="0.01" value={coordinate} on:input={(event) => setPosition(parameter, component, event)} />
              {/each}
            </span>
          {/if}
        </label>
      {/each}
      <button type="button" on:click={() => parameters?.reset()}>Reset parameters</button>
    </section>
  {/if}

  <footer>
    <a href="https://github.com/dyne/frei0r">Explore frei0r on GitHub</a>
    <a href="https://t.me/frei0r">Join the frei0r Telegram</a>
  </footer>
</main>

<style>
  :global(*) {
    box-sizing: border-box;
  }

  :global(body) {
    margin: 0;
    background: #101514;
    color: #f3f6f4;
    font-family: system-ui, sans-serif;
  }

  main {
    display: grid;
    align-content: space-between;
    gap: 2rem;
    min-height: 100vh;
    max-width: 44rem;
    margin: 0 auto;
    padding: 2rem;
  }

  header {
    display: flex;
    flex-wrap: wrap;
    justify-content: space-between;
    gap: 0.75rem 1rem;
  }

  .eyebrow {
    margin: 0;
    color: #93d0ac;
    font-size: 0.875rem;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .status,
  h1,
  p {
    margin: 0;
  }

  .status {
    color: #cad4cf;
    font-size: 0.875rem;
  }

  section {
    display: grid;
    gap: 1rem;
  }

  h1 {
    font-size: clamp(2rem, 7vw, 4rem);
    line-height: 1;
  }

  h2 {
    margin: 0;
    font-size: 1.25rem;
  }

  section > p {
    max-width: 38rem;
    color: #cad4cf;
    font-size: 1.125rem;
    line-height: 1.5;
  }

  footer {
    display: flex;
    flex-wrap: wrap;
    gap: 1rem;
  }

  a {
    display: inline-flex;
    align-items: center;
    min-height: 2.75rem;
    color: #f3f6f4;
    font-weight: 700;
  }

  a:focus-visible {
    outline: 3px solid #93d0ac;
    outline-offset: 3px;
  }

  button {
    justify-self: start;
    min-height: 2.75rem;
    padding: 0.6rem 1rem;
    border: 0;
    border-radius: 0.25rem;
    background: #93d0ac;
    color: #101514;
    font: inherit;
    font-weight: 700;
    cursor: pointer;
  }

  button:disabled {
    cursor: wait;
    opacity: 0.7;
  }

  .processed-frame {
    width: min(100%, 32rem);
    border-radius: 0.5rem;
    background: #050706;
  }

  label,
  .position-control {
    display: grid;
    gap: 0.5rem;
  }
</style>
