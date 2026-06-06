import { defineConfig } from 'vitepress'

const base = process.env.BASE_PATH ?? '/'

export default defineConfig({
  title: 'frei0r',
  description: 'A minimalistic API and portable collection of free and open source video effects.',
  lang: 'en-US',
  base,
  cleanUrls: true,
  srcExclude: [
    'dyne-vitepress/**',
    'content-sources.md',
    'software-sources.md',
    'public-urls.md',
    'README.md',
    'DEPLOYMENT.md'
  ],
  ignoreDeadLinks: [/^\/codedoc\/html\//],
  sitemap: {
    hostname: 'https://frei0r.dyne.org'
  },
  head: [
    ['link', { rel: 'icon', href: `${base}favicon.png` }],
    ['link', { rel: 'canonical', href: 'https://frei0r.dyne.org/' }],
    ['meta', { property: 'og:type', content: 'website' }],
    ['meta', { property: 'og:site_name', content: 'frei0r' }],
    ['meta', { property: 'og:title', content: 'frei0r - free video effects' }],
    ['meta', { property: 'og:description', content: 'A minimalistic API and portable collection of free and open source video effects.' }],
    ['meta', { property: 'og:url', content: 'https://frei0r.dyne.org/' }],
    ['meta', { property: 'og:image', content: 'https://frei0r.dyne.org/pics/frei0r.png' }],
    ['meta', { name: 'twitter:card', content: 'summary_large_image' }]
  ],
  appearance: true,
  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Get frei0r', link: '/get-frei0r' },
      { text: 'Software', link: '/software' },
      { text: 'Documentation', link: '/documentation' },
      { text: 'History', link: '/history' },
      { text: 'Community', link: '/community' },
      { text: 'Discuss', link: 'https://t.me/frei0r' },
      { text: 'About Dyne.org', link: 'https://dyne.org' }
    ],
    socialLinks: [
      { icon: 'github', link: 'https://github.com/dyne/frei0r' }
    ],
    sidebar: [
      {
        text: 'frei0r',
        items: [
          { text: 'Home', link: '/' },
          { text: 'Get frei0r', link: '/get-frei0r' },
          { text: 'Supporting software', link: '/software' },
          { text: 'Community', link: '/community' }
        ]
      },
      {
        text: 'Documentation',
        items: [
          { text: 'Start here', link: '/documentation' },
          { text: 'Using frei0r', link: '/using-frei0r' },
          { text: 'Develop a plugin', link: '/develop-plugin' },
          { text: 'API overview', link: '/api' },
          { text: 'Generated API reference', link: '/codedoc/html/' }
        ]
      },
      {
        text: 'Project',
        items: [
          { text: 'History', link: '/history' },
          { text: 'GitHub', link: 'https://github.com/dyne/frei0r' },
          { text: 'Telegram', link: 'https://t.me/frei0r' }
        ]
      }
    ],
    editLink: {
      pattern: 'https://github.com/dyne/frei0r/edit/master/docs/:path',
      text: 'Edit this page on GitHub'
    },
    search: {
      provider: 'local'
    }
  }
})
