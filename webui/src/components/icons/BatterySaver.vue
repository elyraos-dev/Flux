<template>
  <svg xmlns="http://www.w3.org/2000/svg" :width="size" :height="size" viewBox="0 -960 960 960" :fill="color">
    <path d="M240-160v-640h360v80h80v480h-80v80H240Zm80-80h200v-400H320v400Zm100-60-50-30 20-50h-60l80-180 50 30-20 50h60L400-300Zm-80-180Z"/>
  </svg>
</template>
<script setup>
defineProps({ size: { type: [String, Number], default: 24 }, color: { type: String, default: 'currentColor' } })
</script>
