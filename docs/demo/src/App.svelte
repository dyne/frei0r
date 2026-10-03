<svelte:head>
  <meta
    name="description"
    content="A local, in-browser frei0r video-filter demonstration."
  />
</svelte:head>

<main>
  <header>
    <p class="eyebrow">frei0r browser demo</p>
    <p class="status" aria-live="polite">Preparing the local WebAssembly runtime</p>
  </header>

  <section aria-labelledby="demo-title">
    <h1 id="demo-title">Live filters, running locally</h1>
    <p>
      This demo processes its camera and frei0r runtime in your browser. Video
      and audio are never uploaded.
    </p>
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
</style>
