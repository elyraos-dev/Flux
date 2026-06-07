<template>
  <svg xmlns="http://www.w3.org/2000/svg" :width="size" :height="size" viewBox="0 -960 960 960" :fill="color">
    <path d="M480-80q-83 0-141.5-58.5T280-280q0-48 21-89.5t59-70.5v-400q0-50 35-85t85-35q50 0 85 35t35 85v400q38 29 59 70.5t21 89.5q0 83-58.5 141.5T480-80Zm0-80q50 0 85-35t35-85q0-34-17.5-63T534-390l-54-28v-422q0-17-11.5-28.5T440-880q-17 0-28.5 11.5T400-840v422l-54 28q-31 17-48.5 46T280-280q0 50 35 85t85 35Zm0-120q-17 0-28.5-11.5T440-320v-240h80v240q0 17-11.5 28.5T480-280Z"/>
  </svg>
</template>
<script setup>
defineProps({ size: { type: [String, Number], default: 24 }, color: { type: String, default: 'currentColor' } })
</script>
