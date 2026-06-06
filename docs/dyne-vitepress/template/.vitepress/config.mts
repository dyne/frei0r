import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'Dyne VitePress',
  description: 'Dyne-styled documentation scaffold',
  base: process.env.BASE_PATH ?? '/',
  appearance: true,
  themeConfig: {
    nav: [
      { text: 'Guide', link: '/guide' },
      { text: 'About Dyne.org', link: 'https://dyne.org' }
    ],
    socialLinks: [
      { icon: 'github', link: 'https://github.com/dyne/dyne-vitepress' }
    ],

    sidebar: [
      {
        text: 'Start',
        items: [
          { text: 'Home', link: '/' },
          { text: 'Guide', link: '/guide' },
          { text: 'About', link: '/about' }
        ]
      }
    ]
  }
})
