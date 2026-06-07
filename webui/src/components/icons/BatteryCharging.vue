<template>
  <svg xmlns="http://www.w3.org/2000/svg" :width="size" :height="size" viewBox="0 -960 960 960" :fill="color">
    <path v-if="active" d="M340-240h60l100-200H380l100-200H280v80h100l-100 200h120L340-240Zm-100 80v-480l-40-40v-80h80v-40h200v40h80v80l-40 40v480H240Z"/>
    <path v-else d="M340-240h60l100-200H380l100-200H280v80h100l-100 200h120L340-240Zm-100 80v-480l-40-40v-80h80v-40h200v40h80v80l-40 40v480H240Zm80-80h160v-320H320v320Zm80-160Z"/>
  </svg>
</template>
<script setup>
defineProps({ size: { type: [String, Number], default: 24 }, color: { type: String, default: 'currentColor' }, active: { type: Boolean, default: false } })
</script>
