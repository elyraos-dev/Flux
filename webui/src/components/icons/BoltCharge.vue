<template>
  <svg xmlns="http://www.w3.org/2000/svg" :width="size" :height="size" viewBox="0 -960 960 960" :fill="color">
    <path d="M480-80 200-360l280-280 280 280L480-80ZM280-440h400L480-640 280-440ZM420-160l60-240-180 80 120-400h80l-60 240 180-80-120 400h-80Z"/>
  </svg>
</template>
<script setup>
defineProps({ size: { type: [String, Number], default: 24 }, color: { type: String, default: 'currentColor' } })
</script>
