<template>
  <svg xmlns="http://www.w3.org/2000/svg" :width="size" :height="size" viewBox="0 -960 960 960" :fill="color">
    <path d="M792-56 686-162Q672-120 637-100t-77 20q-42 0-71-29t-29-71h80q0 8 6 14t14 6q8 0 14-6t6-14H430L56-736l56-56 736 736-56 56ZM480-800q-20 0-38.5 5.5T409-778L210-580l50 50v-250h40q0-75 52.5-127.5T480-960q75 0 127.5 52.5T660-780h40v450l-80-80v-370q-17-5-34.5-7.5T480-800ZM160-200v-80l80-80v160H160Zm320-240Z"/>
  </svg>
</template>
<script setup>
defineProps({ size: { type: [String, Number], default: 24 }, color: { type: String, default: 'currentColor' } })
</script>
