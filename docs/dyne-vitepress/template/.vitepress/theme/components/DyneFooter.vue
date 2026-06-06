<template>
  <footer class="dyne-footer">
    <div class="dyne-footer__inner">
      <div class="dyne-footer__content">
        <div class="dyne-footer__top">
          <a class="dyne-footer__logo" href="https://dyne.org" aria-label="Dyne.org">
            <img class="dyne-footer__mark" src="/dyne-mark.svg" alt="" />
            <img class="dyne-footer__logotype" src="/dyne-logotype.svg" alt="Dyne.org" />
          </a>

          <div class="dyne-footer__right">
            <nav class="dyne-footer__socials" aria-label="Dyne.org social links">
              <a
                v-for="social in socialLinks"
                :key="social.name"
                class="dyne-footer__social"
                :href="social.href"
                :aria-label="`Follow Dyne.org on ${social.name}`"
                target="_blank"
                :rel="social.name === 'mastodon' ? 'me noopener' : 'noopener'"
              >
                <SocialIcon :name="social.name" />
              </a>
            </nav>

            <div class="dyne-footer__legal">
              <p>
                Copyleft &copy;1999-{{ currentYear }} Dyne.org foundation<br />
                Haparandadam 7-A1 1013AK, Amsterdam, The Netherlands<br />
                Not for profit org. est. 2005 with reg.num. 34237525 at KvK.NL<br />
                Contents in this website are protected by the International License<br />
                Creative Commons Attribution-NonCommercial-ShareAlike 4.0
              </p>
            </div>
          </div>
        </div>
      </div>
    </div>
  </footer>
</template>

<script setup lang="ts">
import SocialIcon from './SocialIcon.vue'
import { socialLinks } from './social-icons'

const currentYear = new Date().getFullYear()
</script>
