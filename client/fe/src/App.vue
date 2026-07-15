<script setup lang="ts">
import { useAuthStore } from './store/auth'

const authStore = useAuthStore()
</script>

<template>
  <router-view />
</template>
