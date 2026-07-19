import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { getServerInfo, setServerInfo, removeServerInfo } from '../utils/token'

export const useAuthStore = defineStore('auth', () => {
  const serverInfo = ref<{ username: string; host: string; port: number; token: string; balance: number } | null>(
    getServerInfo(),
  )

  const isLoggedIn = computed(() => !!serverInfo.value)
  const username = computed(() => serverInfo.value?.username || '')
  const balance = computed(() => serverInfo.value?.balance || 0)

  function loginSuccess(username: string, host: string, port: number, token: string, balance: number) {
    serverInfo.value = { username, host, port, token, balance }
    setServerInfo({ username, host, port, token, balance })
  }

  function setBalance(b: number) {
    if (serverInfo.value) {
      serverInfo.value.balance = b
      setServerInfo({ ...serverInfo.value, balance: b })
    }
  }

  function logout() {
    serverInfo.value = null
    removeServerInfo()
  }

  return { serverInfo, isLoggedIn, username, balance, loginSuccess, setBalance, logout }
})
