import { create } from 'zustand'
import { getServerInfo, setServerInfo, removeServerInfo } from '../utils/token'

interface AuthState {
  username: string; host: string; port: number; isLoggedIn: boolean
  loginSuccess: (u: string, h: string, p: number) => void
  logout: () => void
}

const saved = getServerInfo()

export const useAuthStore = create<AuthState>((set) => ({
  username: saved?.username || '',
  host: saved?.host || '',
  port: saved?.port || 0,
  isLoggedIn: !!saved,
  loginSuccess: (u, h, p) => {
    setServerInfo({ username: u, host: h, port: p })
    set({ username: u, host: h, port: p, isLoggedIn: true })
  },
  logout: () => {
    removeServerInfo()
    set({ username: '', host: '', port: 0, isLoggedIn: false })
  },
}))
