import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { getToken, setToken, removeToken, getUser, setUser, removeUser } from '../utils/token'

export const useAuthStore = defineStore('auth', () => {
  const token = ref<string | null>(getToken())
  const userInfo = ref<{ uid: string; username: string } | null>(getUser())

  const isLoggedIn = computed(() => !!token.value)

  function loginSuccess(jwt: string, uid: string, username: string) {
    token.value = jwt
    userInfo.value = { uid, username }
    setToken(jwt)
    setUser({ uid, username })
  }

  function logout() {
    token.value = null
    userInfo.value = null
    removeToken()
    removeUser()
  }

  return {
    token,
    userInfo,
    isLoggedIn,
    loginSuccess,
    logout,
  }
})
