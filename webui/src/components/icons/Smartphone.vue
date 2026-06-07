<template>
  <svg xmlns="http://www.w3.org/2000/svg" :width="size" :height="size" viewBox="0 -960 960 960" :fill="color">
    <path d="M300-80q-33 0-56.5-23.5T220-160v-640q0-33 23.5-56.5T300-880h360q33 0 56.5 23.5T740-800v640q0 33-23.5 56.5T660-80H300Zm0-120v40h360v-40H300Zm0-80h360v-480H300v480Zm0-560h360v-40H300v40Zm0 0v-40 40Zm0 640v40-40Z"/>
  </svg>
</template>
<script setup>
defineProps({ size: { type: [String, Number], default: 24 }, color: { type: String, default: 'currentColor' } })
</script>
