<template>
  <svg xmlns="http://www.w3.org/2000/svg" :width="size" :height="size" viewBox="0 -960 960 960" :fill="color">
    <path d="M80-800v-80q117 0 198.5 81.5T360-600h-80q0-83-58.5-141.5T80-800Zm640 0q-83 0-141.5 58.5T520-600h-80q0-117 81.5-198.5T720-880v80ZM160-200v-80l80-80v-200q0-83 50-149.5T420-790v-30q0-25 17.5-42.5T480-880q25 0 42.5 17.5T540-820v30q80 20 130 86.5T720-560v200l80 80v80H160Zm320 120q-33 0-56.5-23.5T400-160h160q0 33-23.5 56.5T480-80Z"/>
  </svg>
</template>
<script setup>
defineProps({ size: { type: [String, Number], default: 24 }, color: { type: String, default: 'currentColor' } })
</script>
