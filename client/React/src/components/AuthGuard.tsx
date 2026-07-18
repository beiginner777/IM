import { Navigate, useLocation } from 'react-router-dom'
import { useAuthStore } from '../store/auth'

export default function AuthGuard({ children }: { children: React.ReactNode }) {
  const { isLoggedIn } = useAuthStore()
  const location = useLocation()
  if (!isLoggedIn) return <Navigate to={`/login?redirect=${encodeURIComponent(location.pathname)}`} replace />
  return <>{children}</>
}
