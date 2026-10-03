<script lang="ts">
  import { onDestroy, onMount, tick } from 'svelte'
  import { CameraService, type CameraSnapshot } from './camera'
  import { controlFixtureFromSearch, feedbackFixtureFromSearch, fixtureCatalogFor, fixtureParametersFor } from './control-fixtures'
  import { composeLiveStatus, hasRapidVisualChanges, qualityMessage } from './feedback-state'
  import { readFilterCatalog, type FilterCatalogItem } from './filter-catalog'
  import { navigateFilterKey } from './filter-navigation'
  import { FilterParameters, type FilterParameter } from './filter-parameters'
  import { FramePipeline } from './frame-pipeline'
  import { FrameScheduler, type FrameSchedulerSnapshot } from './frame-scheduler'
  import { initializeFrei0rDemoRuntime } from './runtime'
  import { resolveStagePresentation, stageFixtureFromSearch, type RuntimeState } from './stage-state'

  interface BeforeInstallPromptEvent extends Event {
    prompt(): Promise<void>
  }

  const camera = new CameraService()
  let cameraState = $state<CameraSnapshot>(camera.snapshot)
  let sourceVideo = $state<HTMLVideoElement>()
  let captureCanvas = $state<HTMLCanvasElement>()
  let outputCanvas = $state<HTMLCanvasElement>()
  let scheduler: FrameScheduler | undefined
  let parameters: FilterParameters | undefined
  let filterParameters = $state.raw<readonly FilterParameter[]>([])
  let filterCatalog = $state.raw<readonly FilterCatalogItem[]>([])
  let activeFilterIndex = $state(0)
  let parametersExpanded = $state(false)
  let schedulerStatus = 'Preparing the local WebAssembly runtime.'
  let schedulerSnapshot = $state.raw<FrameSchedulerSnapshot>({
    active: false,
    status: schedulerStatus,
    renderedFrames: 0,
    droppedFrames: 0,
    qualityScale: 1
  })
  let runtimeState = $state<RuntimeState>('loading')
  let runtimeError = $state<string>()
  let cameraWasStarted = $state(false)
  let online = $state(globalThis.navigator?.onLine ?? true)
  let installPrompt = $state<BeforeInstallPromptEvent>()
  let installed = $state(false)
  let unsubscribeScheduler = () => {}
  let unsubscribeParameters = () => {}
  let unsubscribeConfiguration = () => {}
  const unsubscribe = camera.subscribe((snapshot) => {
    cameraState = snapshot
    if (snapshot.status !== 'active') scheduler?.stop()
  })
  const stageFixture = stageFixtureFromSearch(globalThis.location?.search ?? '', import.meta.env.DEV)
  const controlFixture = controlFixtureFromSearch(globalThis.location?.search ?? '', import.meta.env.DEV)
  const feedbackFixture = feedbackFixtureFromSearch(globalThis.location?.search ?? '', import.meta.env.DEV)
  let visibleCatalog = $derived(controlFixture ? fixtureCatalogFor(controlFixture) : filterCatalog)
  let visibleParameters = $derived(controlFixture ? fixtureParametersFor(controlFixture) ?? [] : filterParameters)
  let activeFilter = $derived(visibleCatalog.find((filter) => filter.index === activeFilterIndex) ?? visibleCatalog[0])
  let isOnline = $derived(feedbackFixture === 'offline' ? false : online)
  let previewQuality = $derived(feedbackFixture === 'reduced-quality' ? 0.75 : schedulerSnapshot.qualityScale)
  let rapidVisualChanges = $derived(feedbackFixture === 'flashing' || hasRapidVisualChanges(activeFilter?.id))
  let previewQualityMessage = $derived(qualityMessage(previewQuality))
  let installAvailable = $derived(feedbackFixture === 'installable' || Boolean(installPrompt))

  $effect(() => {
    if (sourceVideo) sourceVideo.srcObject = (cameraState.stream as MediaStream | undefined) ?? null
  })

  let stage = $derived(resolveStagePresentation({
    camera: cameraState,
    runtimeState,
    runtimeError,
    scheduler: schedulerSnapshot,
    cameraWasStarted,
    fixture: stageFixture
  }))
  let statusAnnouncement = $derived(composeLiveStatus({
    stageStatus: stage.status,
    online: isOnline,
    qualityScale: previewQuality,
    rapidVisualChanges
  }))

  onMount(() => {
    const markInstalled = () => installed = true
    window.addEventListener('appinstalled', markInstalled)
    if (!stageFixture) void initializeEngine()
    return () => {
      window.removeEventListener('appinstalled', markInstalled)
      scheduler?.stop()
      unsubscribeScheduler()
      unsubscribeParameters()
      unsubscribeConfiguration()
    }
  })

  async function initializeEngine() {
    try {
      const video = sourceVideo
      const capture = captureCanvas
      const output = outputCanvas
      if (!video || !capture || !output) {
        throw new Error('The camera preview surface could not be prepared.')
      }
      const runtime = await initializeFrei0rDemoRuntime()
      const pipeline = new FramePipeline(runtime, { captureCanvas: capture, outputCanvas: output })
      scheduler = new FrameScheduler(pipeline, video)
      parameters = new FilterParameters(runtime)
      filterCatalog = readFilterCatalog(runtime)
      scheduler.selectFilter(0)
      unsubscribeScheduler = scheduler.subscribe((snapshot) => {
        schedulerStatus = snapshot.status
        schedulerSnapshot = snapshot
      })
      unsubscribeParameters = parameters.subscribe((nextParameters) => {
        filterParameters = nextParameters
      })
      unsubscribeConfiguration = scheduler.subscribeConfiguration((catalogIndex) => {
        activeFilterIndex = catalogIndex
        parameters?.refresh()
      })
      if (cameraState.status === 'active') scheduler.start()
      runtimeState = 'ready'
    } catch (error) {
      runtimeError = error instanceof Error ? error.message : 'The local filter runtime could not start.'
      schedulerStatus = runtimeError
      runtimeState = 'failure'
    }
  }

  async function toggleCamera() {
    if (stageFixture || runtimeState !== 'ready') return
    if (cameraState.status === 'active' || cameraState.status === 'starting') {
      scheduler?.stop()
      camera.stop()
      return
    }
    const result = await camera.start()
    if (result.ok) {
      cameraWasStarted = true
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

  function chooseFilter(index: number, focus = false) {
    if (!visibleCatalog.some((filter) => filter.index === index)) return
    activeFilterIndex = index
    scheduler?.selectFilter(index)
    void tick().then(() => {
      const activeControl = document.getElementById(`filter-${index}`)
      activeControl?.scrollIntoView({
        behavior: globalThis.matchMedia?.('(prefers-reduced-motion: reduce)').matches ? 'auto' : 'smooth',
        block: 'nearest',
        inline: 'center'
      })
      if (focus) activeControl?.focus()
    })
  }

  function chooseAdjacentFilter(direction: -1 | 1) {
    if (visibleCatalog.length === 0) return
    const current = Math.max(0, visibleCatalog.findIndex((filter) => filter.index === activeFilterIndex))
    chooseFilter(visibleCatalog[(current + direction + visibleCatalog.length) % visibleCatalog.length].index)
  }

  function handleFilterKeydown(event: KeyboardEvent, index: number) {
    if (navigateFilterKey(event.key, index, visibleCatalog.map((filter) => filter.index), (nextIndex) => chooseFilter(nextIndex, true))) {
      event.preventDefault()
    }
  }

  function resetParameters() {
    parameters?.reset()
  }

  function recordInstallPrompt(event: Event) {
    event.preventDefault()
    installPrompt = event as BeforeInstallPromptEvent
  }

  async function installDemo() {
    if (!installPrompt) return
    await installPrompt.prompt()
    installPrompt = undefined
  }

  onDestroy(() => {
    unsubscribe()
    camera.destroy()
  })
</script>

<svelte:window
  ononline={() => online = true}
  onoffline={() => online = false}
  onbeforeinstallprompt={recordInstallPrompt}
/>

<svelte:head>
  <meta
    name="description"
    content="A local, in-browser frei0r video-filter demonstration."
  />
</svelte:head>

<main class="demo-shell" data-stage={stage.kind}>
  <header class="app-header">
    <a class="wordmark" href={import.meta.env.BASE_URL}>frei0r <span>live</span></a>
    <div class="status-cluster">
      {#if stage.cameraActive}
        <span class="camera-active"><span aria-hidden="true"></span>Camera active</span>
      {/if}
      <p class="status">{stage.status}</p>
    </div>
  </header>

  <p class="visually-hidden" role="status" aria-live="polite" aria-atomic="true">{statusAnnouncement}</p>

  {#if !isOnline || installAvailable || installed}
    <aside class="connection-status" aria-label="Install and offline status">
      {#if !isOnline}<p><strong>Offline.</strong> Cached demo files remain available when already stored on this device.</p>{/if}
      {#if installed}<p>Installed as an app on this device.</p>{/if}
      {#if installAvailable}
        {#if installPrompt}
          <button type="button" onclick={installDemo}>Install demo</button>
        {:else}
          <p>Install available in a supporting browser.</p>
        {/if}
      {/if}
    </aside>
  {/if}

  <section class="live-stage" aria-labelledby="demo-title">
    <div class="stage-intro">
      <h1 id="demo-title">Live filters, running locally</h1>
      <p>This demo processes frei0r effects in your browser. Video and audio are never uploaded.</p>
    </div>
    <div class="stage-frame" aria-busy={stage.kind === 'requesting'}>
    <video bind:this={sourceVideo} autoplay muted playsinline aria-label="Camera source" hidden></video>
    <canvas bind:this={captureCanvas} hidden></canvas>
      <canvas bind:this={outputCanvas} class="processed-frame" aria-label="Filtered camera preview" hidden={!stage.showCanvas}></canvas>
      <div class="stage-overlay">
        <div>
          <h2>{stage.title}</h2>
          <p>{stage.detail}</p>
        </div>
        {#if stage.action !== 'none'}
          <button class="stage-action" type="button" onclick={toggleCamera} disabled={stage.actionDisabled || Boolean(stageFixture)}>
            {stage.actionLabel}
          </button>
        {/if}
      </div>
    </div>
    {#if stage.cameraActive}
      <div class="stage-feedback">
        <p class="stage-note">The camera stays active until you stop it or leave this page.</p>
        {#if previewQualityMessage}<p class="quality-notice">{previewQualityMessage}</p>{/if}
      </div>
    {/if}
  </section>

  <section class="action-dock" aria-labelledby="filter-dock-title">
    <div class="dock-heading">
      <div>
        <h2 id="filter-dock-title">Choose a filter</h2>
        {#if activeFilter}
          <p class="filter-description"><strong>{activeFilter.name}</strong> — {activeFilter.explanation}</p>
          {#if rapidVisualChanges}<p class="rapid-change-warning"><strong>Rapid visual changes possible.</strong> Pause or choose another filter if this is uncomfortable.</p>{/if}
        {:else}
          <p class="filter-description">Loading the local frei0r filter catalog.</p>
        {/if}
      </div>
      {#if visibleCatalog.length > 1}
        <div class="filter-stepper" aria-label="Adjacent filters">
          <button type="button" onclick={() => chooseAdjacentFilter(-1)} aria-label="Previous filter">Previous</button>
          <span aria-hidden="true">{Math.max(1, visibleCatalog.findIndex((filter) => filter.index === activeFilterIndex) + 1)} / {visibleCatalog.length}</span>
          <button type="button" onclick={() => chooseAdjacentFilter(1)} aria-label="Next filter">Next</button>
        </div>
      {/if}
    </div>

    {#if visibleCatalog.length > 0}
      <div class="filter-rail" aria-label="Available frei0r filters" role="group">
        {#each visibleCatalog as filter (filter.index)}
          <button
            id={`filter-${filter.index}`}
            class:active={filter.index === activeFilterIndex}
            type="button"
            aria-pressed={filter.index === activeFilterIndex}
            aria-label={`${filter.name}, ${filter.parameterCount} ${filter.parameterCount === 1 ? 'parameter' : 'parameters'}`}
            onclick={() => chooseFilter(filter.index)}
            onkeydown={(event) => handleFilterKeydown(event, filter.index)}
          >
            <span>{filter.name}</span>
            <small>{filter.parameterCount} {filter.parameterCount === 1 ? 'parameter' : 'parameters'}</small>
            {#if filter.index === activeFilterIndex}<small class="active-label">Selected</small>{/if}
          </button>
        {/each}
      </div>
    {/if}

    <section class="parameter-panel" aria-labelledby="parameter-title">
      <button
        class="parameter-toggle"
        type="button"
        aria-expanded={parametersExpanded}
        aria-controls="parameter-content"
        onclick={() => parametersExpanded = !parametersExpanded}
      >
        <span id="parameter-title">Parameters</span>
        <span>{visibleParameters.length} {visibleParameters.length === 1 ? 'control' : 'controls'}</span>
      </button>
      {#if parametersExpanded}
        <div id="parameter-content" class="parameter-content">
          {#if visibleParameters.length === 0}
            <p class="parameter-empty">This filter has no adjustable parameters.</p>
          {:else}
            {#each visibleParameters as parameter (parameter.index)}
        <label title={parameter.explanation}>
          <span>{parameter.name}</span>
          {#if parameter.kind === 'boolean'}
            <input type="checkbox" checked={parameter.value} onchange={(event) => setBoolean(parameter, event)} />
          {:else if parameter.kind === 'number'}
            <input type="range" min="0" max="1" step="0.01" value={parameter.value} oninput={(event) => setNumber(parameter, event)} />
          {:else if parameter.kind === 'color'}
            <input type="color" value={colorValue(parameter.value)} oninput={(event) => setColor(parameter, event)} />
          {:else}
            <span class="position-control">
              {#each parameter.value as coordinate, component (component === 0 ? 'horizontal' : 'vertical')}
                <input aria-label={`${parameter.name} ${component === 0 ? 'horizontal' : 'vertical'}`} type="range" min="0" max="1" step="0.01" value={coordinate} oninput={(event) => setPosition(parameter, component, event)} />
              {/each}
            </span>
          {/if}
        </label>
            {/each}
            <button type="button" onclick={resetParameters}>Reset parameters</button>
          {/if}
        </div>
      {/if}
    </section>
  </section>

  <footer>
    <a href="https://github.com/dyne/frei0r">Explore frei0r on GitHub</a>
    <a href="https://t.me/frei0r">Join the frei0r Telegram</a>
  </footer>
</main>

<style>
  :global(*) {
    box-sizing: border-box;
  }

  :global(:root) {
    color-scheme: light;
  }

  :global(body) {
    margin: 0;
    background: #ffeedd;
    color: #251e18;
    font-family: Inter, "Segoe UI", Arial, sans-serif;
  }

  :global(::selection) {
    background: #cb743b;
    color: #fffaf6;
  }

  .demo-shell {
    display: grid;
    align-content: start;
    gap: clamp(1.25rem, 3vw, 2.5rem);
    min-height: 100vh;
    max-width: 72rem;
    margin: 0 auto;
    padding: clamp(1rem, 4vw, 3rem);
  }

  .demo-shell > *,
  .stage-overlay > *,
  .action-dock,
  .dock-heading,
  .filter-stepper,
  .parameter-panel,
  .parameter-toggle {
    min-width: 0;
  }

  .app-header {
    display: flex;
    flex-wrap: wrap;
    justify-content: space-between;
    align-items: center;
    gap: 0.75rem 1.25rem;
  }

  .wordmark {
    margin: 0;
    color: #251e18;
    font-family: Syne, Inter, "Segoe UI", Arial, sans-serif;
    font-size: 1.125rem;
    font-weight: 800;
    letter-spacing: -0.03em;
    text-decoration: none;
  }

  .wordmark span {
    color: #8d4720;
  }

  .status-cluster {
    display: grid;
    justify-items: end;
    gap: 0.25rem;
  }

  .status,
  h1,
  h2,
  p {
    margin: 0;
  }

  .visually-hidden {
    position: absolute;
    width: 1px;
    height: 1px;
    padding: 0;
    margin: -1px;
    overflow: hidden;
    clip: rect(0, 0, 0, 0);
    white-space: nowrap;
    border: 0;
  }

  .status {
    color: #5f5449;
    font-size: 0.875rem;
    text-align: end;
  }

  .connection-status,
  .stage-feedback {
    display: grid;
    gap: 0.5rem;
  }

  .connection-status {
    grid-template-columns: minmax(0, 1fr) auto;
    align-items: center;
    max-width: 46rem;
    justify-self: center;
    width: 100%;
    padding: 0.75rem 0;
    border-block: 1px solid rgba(37, 30, 24, 0.2);
    color: #5f5449;
    line-height: 1.45;
  }

  .connection-status p {
    min-width: 0;
  }

  .connection-status strong,
  .rapid-change-warning strong {
    color: #251e18;
  }

  .camera-active {
    display: inline-flex;
    align-items: center;
    gap: 0.45rem;
    color: #1b564d;
    font-size: 0.875rem;
    font-weight: 700;
  }

  .camera-active span {
    width: 0.7rem;
    height: 0.7rem;
    border: 2px solid currentColor;
    border-radius: 50%;
  }

  .live-stage,
  .action-dock,
  .parameter-panel,
  .parameter-content {
    display: grid;
    gap: 1.125rem;
  }

  .stage-intro {
    display: grid;
    gap: 0.5rem;
    max-width: 42rem;
  }

  h1 {
    font-family: Syne, Inter, "Segoe UI", Arial, sans-serif;
    font-size: clamp(2rem, 5vw, 3.75rem);
    letter-spacing: -0.04em;
    line-height: 1.02;
  }

  h2 {
    margin: 0;
    font-family: Syne, Inter, "Segoe UI", Arial, sans-serif;
    font-size: clamp(1.25rem, 3vw, 1.75rem);
    letter-spacing: -0.03em;
    line-height: 1.1;
  }

  .stage-intro p,
  .stage-overlay p,
  .stage-note {
    color: #5f5449;
    line-height: 1.5;
  }

  .stage-frame {
    position: relative;
    display: grid;
    align-items: end;
    width: 100%;
    max-width: 46rem;
    justify-self: center;
    aspect-ratio: 4 / 3;
    min-height: 0;
    overflow: hidden;
    border: 1px solid rgba(37, 30, 24, 0.16);
    border-radius: 16px;
    background: #1e1815;
    box-shadow: 0 18px 34px rgba(73, 48, 32, 0.16);
  }

  .processed-frame {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .stage-overlay {
    z-index: 1;
    display: grid;
    grid-template-columns: minmax(0, 1fr) auto;
    align-items: end;
    gap: 1rem;
    min-height: 100%;
    padding: clamp(1rem, 4vw, 2rem);
    background: linear-gradient(to top, rgba(30, 24, 21, 0.92), rgba(30, 24, 21, 0.1) 62%);
    color: #fff4e8;
  }

  .stage-frame:has(.processed-frame:not([hidden])) .stage-overlay {
    position: absolute;
    inset: 0;
  }

  .stage-overlay p {
    max-width: 36rem;
    margin-top: 0.6rem;
    color: #ead8ca;
  }

  .stage-action,
  button {
    min-height: 2.75rem;
    padding: 0.65rem 1rem;
    border: 0;
    border-radius: 0.5rem;
    background: #a95025;
    color: #fffaf6;
    font: inherit;
    font-weight: 700;
    cursor: pointer;
  }

  .stage-action {
    align-self: end;
    box-shadow: 0 8px 18px rgba(0, 0, 0, 0.24);
  }

  .stage-note {
    font-size: 0.9375rem;
  }

  .quality-notice,
  .rapid-change-warning {
    color: #5f5449;
    line-height: 1.5;
  }

  .quality-notice {
    color: #1b564d;
  }

  .action-dock {
    max-width: 46rem;
    justify-self: center;
    width: 100%;
    grid-template-columns: minmax(0, 1fr);
    padding: clamp(1rem, 3vw, 1.5rem);
    border: 1px solid rgba(37, 30, 24, 0.16);
    border-radius: 16px;
    background: rgba(255, 250, 246, 0.58);
    box-shadow: 0 12px 24px rgba(73, 48, 32, 0.08);
  }

  .dock-heading {
    display: grid;
    gap: 0.875rem;
  }

  .filter-description {
    max-width: 42rem;
    margin-top: 0.45rem;
    color: #5f5449;
    line-height: 1.5;
  }

  .filter-description strong {
    color: #251e18;
  }

  .rapid-change-warning {
    margin-top: 0.5rem;
  }

  .filter-stepper {
    display: grid;
    grid-template-columns: minmax(2.75rem, 1fr) auto minmax(2.75rem, 1fr);
    align-items: center;
    gap: 0.5rem;
  }

  .filter-stepper span {
    color: #5f5449;
    font-size: 0.875rem;
    font-variant-numeric: tabular-nums;
    text-align: center;
  }

  .filter-stepper button {
    min-width: 0;
    overflow-wrap: anywhere;
  }

  .filter-rail {
    display: flex;
    gap: 0.65rem;
    margin-inline: calc(clamp(1rem, 3vw, 1.5rem) * -1);
    padding: 0.2rem clamp(1rem, 3vw, 1.5rem) 0.65rem;
    overflow-x: auto;
    overscroll-behavior-inline: contain;
    scroll-padding-inline: clamp(1rem, 3vw, 1.5rem);
    scroll-snap-type: inline mandatory;
  }

  .filter-rail button {
    display: grid;
    flex: 0 0 auto;
    gap: 0.2rem;
    min-width: 8.75rem;
    min-height: 4.5rem;
    padding: 0.7rem 0.85rem;
    border: 1px solid rgba(37, 30, 24, 0.25);
    background: #fffaf6;
    color: #251e18;
    scroll-snap-align: center;
    text-align: start;
  }

  .filter-rail button.active {
    border-width: 2px;
    border-color: #1b564d;
    box-shadow: inset 0 -0.3rem #257265;
  }

  .filter-rail small {
    color: #5f5449;
    font-size: 0.75rem;
    font-weight: 600;
  }

  .filter-rail .active-label {
    color: #1b564d;
    font-weight: 800;
  }

  .parameter-panel {
    gap: 0.75rem;
    grid-template-columns: minmax(0, 1fr);
    justify-self: stretch;
    width: 100%;
  }

  .parameter-toggle {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 0.35rem 0.75rem;
    width: 100%;
    background: #251e18;
    color: #fffaf6;
    text-align: start;
  }

  .parameter-toggle span:last-child {
    color: #ead8ca;
    font-size: 0.875rem;
    font-variant-numeric: tabular-nums;
    overflow-wrap: anywhere;
  }

  .parameter-content {
    padding: 0.25rem 0;
    grid-template-columns: minmax(0, 1fr);
  }

  .parameter-empty {
    color: #5f5449;
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
    color: #8d4720;
    font-weight: 700;
  }

  a:focus-visible {
    outline: 3px solid #257265;
    outline-offset: 3px;
  }

  button:hover:not(:disabled) {
    background: #8d4720;
  }

  .filter-rail button:hover:not(:disabled) {
    background: #fff4e8;
  }

  .parameter-toggle:hover:not(:disabled) {
    background: #3d3028;
  }

  button:disabled {
    cursor: wait;
    opacity: 0.7;
  }

  label,
  .position-control {
    display: grid;
    gap: 0.5rem;
    min-width: 0;
  }

  input[type='range'] {
    width: 100%;
    min-width: 0;
    max-width: 100%;
  }

  button:focus-visible,
  input:focus-visible {
    outline: 3px solid #257265;
    outline-offset: 3px;
  }

  @media (prefers-reduced-motion: reduce) {
    :global(*) {
      scroll-behavior: auto !important;
      transition-duration: 0.01ms !important;
      animation-duration: 0.01ms !important;
      animation-iteration-count: 1 !important;
    }
  }

  @media (forced-colors: active) {
    .stage-frame,
    .action-dock,
    .connection-status,
    .filter-rail button,
    .parameter-toggle {
      border: 1px solid CanvasText;
      box-shadow: none;
    }

    .filter-rail button.active {
      border: 3px solid Highlight;
      box-shadow: none;
    }

    button:focus-visible,
    input:focus-visible,
    a:focus-visible {
      outline-color: Highlight;
    }
  }

  @media (max-width: 30rem) {
    .stage-overlay {
      grid-template-columns: 1fr;
    }

    .stage-action {
      justify-self: start;
    }

    .status-cluster {
      justify-items: start;
    }

    .status {
      text-align: start;
    }

    .connection-status {
      grid-template-columns: 1fr;
    }

    .filter-stepper {
      grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
    }

    .filter-stepper span {
      grid-column: 1 / -1;
      grid-row: 2;
    }
  }

  @media (min-width: 48rem) {
    .live-stage {
      gap: 1.5rem;
    }

    .dock-heading {
      grid-template-columns: minmax(0, 1fr) minmax(15rem, 18rem);
      align-items: end;
    }
  }
</style>
