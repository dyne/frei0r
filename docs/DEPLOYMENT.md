# Website deployment

The repository workflow `.github/workflows/pages.yml` builds VitePress and
Doxygen, uploads `docs/.vitepress/dist`, and deploys it with GitHub Pages.

## One-time repository configuration

1. Open **Settings → Pages** in `dyne/frei0r`.
2. Set **Source** to **GitHub Actions**.
3. Set the custom domain to `frei0r.dyne.org`.
4. Wait for GitHub's DNS check to pass.
5. Enable **Enforce HTTPS**.

The workflow uses the protected `github-pages` environment. Repository
administrators may add deployment reviewers or branch rules there.

`docs/public/CNAME` preserves the domain in the static artifact and alternate
publishing tools, but the GitHub Pages repository setting is authoritative for
an Actions deployment.

## DNS

Before cutover, confirm that `frei0r.dyne.org` resolves to the existing GitHub
Pages endpoint:

```powershell
Resolve-DnsName frei0r.dyne.org
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
https://frei0r.dyne.org/
https://frei0r.dyne.org/pics/fla_name_lb.webp
https://frei0r.dyne.org/pics/frei0r.png
https://frei0r.dyne.org/frei0r-all.webm
https://frei0r.dyne.org/codedoc/html/
https://frei0r.dyne.org/codedoc/html/frei0r_8h.html
```

Check that:

- HTTP redirects to HTTPS.
- Page metadata names `https://frei0r.dyne.org` as canonical.
- VitePress navigation, assets and local search work.
- Doxygen pages load their CSS, scripts and diagrams.
- GitHub and Telegram links lead to the intended project destinations.

Keep `gh_pages` until these checks have passed in production. It is historical
content and a rollback source, not the new publication mechanism.
