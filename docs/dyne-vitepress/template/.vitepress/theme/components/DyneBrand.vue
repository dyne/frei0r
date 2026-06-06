<template>
  <span class="dyne-brand" :class="variant">
    <img class="dyne-brand__mark" src="/dyne-mark.svg" alt="Dyne.org" />
    <img class="dyne-brand__logotype" src="/dyne-logotype.svg" alt="foundation" />
  </span>
</template>

<script setup lang="ts">
defineProps<{
  variant?: 'nav' | 'doc'
}>()
</script>

