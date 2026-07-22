import { create } from 'zustand'
import { getServerInfo, setServerInfo, removeServerInfo } from '../utils/token'
import { resetBaseURL } from '../api/request'

interface AuthState {
  username: string; host: string; port: number; isLoggedIn: boolean
  loginSuccess: (u: string, h: string, p: number, t?: string, b?: number) => void
  logout: () => void
}

const saved = getServerInfo()

export const useAuthStore = create<AuthState>((set) => ({
  username: saved?.username || '',
  host: saved?.host || '',
  port: saved?.port || 0,
  isLoggedIn: !!saved,
  loginSuccess: (u, h, p, t, b) => {
    setServerInfo({ username: u, host: h, port: p, token: t, balance: b })
    set({ username: u, host: h, port: p, isLoggedIn: true })
  },
  logout: () => {
    removeServerInfo()
    resetBaseURL()
    set({ username: '', host: '', port: 0, isLoggedIn: false })
  },
}))
