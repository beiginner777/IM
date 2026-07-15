const TOKEN_KEY = 'im_jwt'
const USER_KEY = 'im_user'

export function getToken(): string | null {
  return localStorage.getItem(TOKEN_KEY)
}

export function setToken(token: string): void {
  localStorage.setItem(TOKEN_KEY, token)
}

export function removeToken(): void {
  localStorage.removeItem(TOKEN_KEY)
}

export function getUser(): { uid: string; username: string } | null {
  const raw = localStorage.getItem(USER_KEY)
  if (!raw) return null
  try {
    return JSON.parse(raw)
  } catch {
    return null
  }
}

export function setUser(user: { uid: string; username: string }): void {
  localStorage.setItem(USER_KEY, JSON.stringify(user))
}

export function removeUser(): void {
  localStorage.removeItem(USER_KEY)
}

export function isLoggedIn(): boolean {
  return !!getToken()
}

export function logout(): void {
  removeToken()
  removeUser()
}
