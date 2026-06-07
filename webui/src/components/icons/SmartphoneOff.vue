<template>
  <svg xmlns="http://www.w3.org/2000/svg" :width="size" :height="size" viewBox="0 -960 960 960" :fill="color">
    <path d="M792-56 56-792l56-56 736 736-56 56ZM660-80H300q-33 0-56.5-23.5T220-160v-80h80v80h360v-40h-82l-80-80H220v-400L100-800l56-56 694 694q6-13 8-27t2-27v-584H340l-80-80h400q33 0 56.5 23.5T740-800v640q0 33-23.5 56.5T660-80ZM480-200q17 0 28.5-11.5T520-240q0-17-11.5-28.5T480-280q-17 0-28.5 11.5T440-240q0 17 11.5 28.5T480-200Z"/>
  </svg>
</template>
<script setup>
defineProps({ size: { type: [String, Number], default: 24 }, color: { type: String, default: 'currentColor' } })
</script>
