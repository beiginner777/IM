import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { getServerInfo, setServerInfo, removeServerInfo } from '../utils/token'

export const useAuthStore = defineStore('auth', () => {
  const serverInfo = ref<{ username: string; host: string; port: number } | null>(
    getServerInfo(),
  )

  const isLoggedIn = computed(() => !!serverInfo.value)
  const username = computed(() => serverInfo.value?.username || '')

  function loginSuccess(username: string, host: string, port: number) {
    serverInfo.value = { username, host, port }
    setServerInfo({ username, host, port })
  }

  function logout() {
    serverInfo.value = null
    removeServerInfo()
  }

  return {
    serverInfo,
    isLoggedIn,
    username,
    loginSuccess,
    logout,
  }
})
