<template>
  <svg xmlns="http://www.w3.org/2000/svg" :width="size" :height="size" viewBox="0 -960 960 960" :fill="color">
    <path d="M792-56 671-177 480-368v252L280-316H120v-240h160L56-792l56-56 736 736-56 56Zm-32-372-58-58q17-31 25.5-65t8.5-70q0-94-55-168T560-849v-82q124 28 202 125.5T840-481q0 53-14.5 102T760-428ZM650-538l-90-90v-152q47 22 73.5 66t26.5 96q0 15-2.5 29.5T650-538ZM480-606l-86 86-74-74 160-161v149Z"/>
  </svg>
</template>
<script setup>
defineProps({ size: { type: [String, Number], default: 24 }, color: { type: String, default: 'currentColor' } })
</script>
