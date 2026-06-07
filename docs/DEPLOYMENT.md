# Website deployment

The repository workflow `.github/workflows/pages.yml` builds VitePress and
Doxygen, uploads `docs/.vitepress/dist`, and deploys it with GitHub Pages.

## One-time repository configuration

1. Open **Settings → Pages** in `dyne/frei0r`.
2. Set **Source** to **GitHub Actions**.
3. Keep the public site configured for the `/frei0r/` path on `dyne.org`.
4. If you later move the site back to the `frei0r.dyne.org` custom domain,
   set `BASE_PATH=/` before deployment.
5. Enable **Enforce HTTPS** on the live site if the platform exposes that toggle.

The workflow uses the protected `github-pages` environment. Repository
administrators may add deployment reviewers or branch rules there.

`docs/public/CNAME` preserves the custom-domain value in the static artifact and
alternate publishing tools, but it is not used for the current subpath
deployment.

## DNS

Before cutover, confirm that `dyne.org/frei0r/` resolves and serves the site
you expect:

```powershell
Resolve-DnsName dyne.org
```

Do not change DNS and the publishing source at the same time unless rollback
has been prepared. The existing `gh_pages` branch remains available during the
first deployment.

## Manual validation

Run the website workflow with **workflow_dispatch** on `master`, or merge a
documentation change after Pages is configured. Confirm the build job contains:

- `docs/.vitepress/dist/index.html`
- `docs/.vitepress/dist/codedoc/html/index.html`

After deployment, verify:

```text
https://dyne.org/frei0r/
https://dyne.org/frei0r/pics/fla_name_lb.webp
https://dyne.org/frei0r/pics/frei0r.png
https://dyne.org/frei0r/frei0r-all.webm
https://dyne.org/frei0r/codedoc/html/
https://dyne.org/frei0r/codedoc/html/frei0r_8h.html
```

Check that:

- HTTP redirects to HTTPS.
- Page metadata names `https://dyne.org/frei0r/` as canonical.
- VitePress navigation, assets and local search work.
- Doxygen pages load their CSS, scripts and diagrams.
- GitHub and Telegram links lead to the intended project destinations.

Keep `gh_pages` until these checks have passed in production. It is historical
content and a rollback source, not the new publication mechanism.
