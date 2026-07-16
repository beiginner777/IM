const SERVER_KEY = 'im_server'

interface ServerInfo {
  username: string
  host: string
  port: number
}

export function getServerInfo(): ServerInfo | null {
  const raw = localStorage.getItem(SERVER_KEY)
  if (!raw) return null
  try {
    return JSON.parse(raw)
  } catch {
    return null
  }
}

export function setServerInfo(info: ServerInfo): void {
  localStorage.setItem(SERVER_KEY, JSON.stringify(info))
}

export function removeServerInfo(): void {
  localStorage.removeItem(SERVER_KEY)
}

export function isLoggedIn(): boolean {
  return !!getServerInfo()
}

export function logout(): void {
  removeServerInfo()
}
