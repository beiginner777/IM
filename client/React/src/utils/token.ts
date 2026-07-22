const SERVER_KEY = 'im_server'

export interface ServerInfo { username: string; host: string; port: number }

export function getServerInfo(): ServerInfo | null {
  const raw = localStorage.getItem(SERVER_KEY)
  if (!raw) return null
  try { return JSON.parse(raw) } catch { return null }
}
export function setServerInfo(info: ServerInfo) { localStorage.setItem(SERVER_KEY, JSON.stringify(info)) }
export function removeServerInfo() { localStorage.removeItem(SERVER_KEY) }
export function isLoggedIn() { return !!getServerInfo() }
export function logout() { removeServerInfo() }
