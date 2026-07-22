import { create } from 'zustand'
import { getServerInfo, setServerInfo, removeServerInfo } from '../utils/token'
import { resetBaseURL } from '../api/request'

interface AuthState {
  username: string; host: string; port: number; token: string; balance: number; isLoggedIn: boolean
  loginSuccess: (u: string, h: string, p: number, t: string, b: number) => void
  setBalance: (b: number) => void
  logout: () => void
}

const saved = getServerInfo()

export const useAuthStore = create<AuthState>((set) => ({
  username: saved?.username || '',
  host: saved?.host || '',
  port: saved?.port || 0,
  token: saved?.token || '',
  balance: saved?.balance || 0,
  isLoggedIn: !!saved,
  loginSuccess: (u, h, p, t, b) => {
    setServerInfo({ username: u, host: h, port: p, token: t, balance: b })
    set({ username: u, host: h, port: p, token: t, balance: b, isLoggedIn: true })
  },
  setBalance: (b) => {
    const s = getServerInfo()
    if (s) { s.balance = b; setServerInfo(s) }
    set({ balance: b })
  },
  logout: () => { removeServerInfo(); resetBaseURL(); set({ username: '', host: '', port: 0, token: '', balance: 0, isLoggedIn: false }) },
}))
