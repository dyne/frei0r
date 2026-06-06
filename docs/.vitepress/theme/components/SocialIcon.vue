<template>
  <svg
    class="dyne-social-icon"
    width="30"
    height="30"
    viewBox="0 0 24 24"
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
    aria-hidden="true"
  >
    <g v-html="icon"></g>
  </svg>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { socialIcons, type SocialIconName } from './social-icons'

const props = defineProps<{
  name: SocialIconName
}>()

const icon = computed(() => socialIcons[props.name])
</script>
