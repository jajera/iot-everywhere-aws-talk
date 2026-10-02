<script setup lang="ts">
defineProps<{
  image?: string
}>()
</script>

<template>
  <div class="cover-photo relative h-full w-full overflow-hidden">
    <img
      :src="image || '/cover.png'"
      alt=""
      class="absolute inset-0 h-full w-full object-contain"
    >
    <div class="absolute inset-0 z-10">
      <slot />
    </div>
  </div>
</template>

<style scoped>
.cover-photo {
  background: #0b1219;
}
</style>
