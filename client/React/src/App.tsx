import { lazy, Suspense } from 'react'
import { Routes, Route } from 'react-router-dom'
import { ConfigProvider, theme, App as AntApp, Spin } from 'antd'
import Navbar from './components/Navbar'
import AuthGuard from './components/AuthGuard'

const HomePage      = lazy(() => import('./pages/HomePage'))
const LoginPage     = lazy(() => import('./pages/LoginPage'))
const RegisterPage  = lazy(() => import('./pages/RegisterPage'))
const ProductsPage  = lazy(() => import('./pages/ProductListPage'))
const OrdersPage    = lazy(() => import('./pages/OrdersPage'))
const RankPage      = lazy(() => import('./pages/RankPage'))

function PageLoader() {
  return <div style={{ display: 'flex', justifyContent: 'center', alignItems: 'center', minHeight: 'calc(100vh - 56px)' }}><Spin size="large" /></div>
}

export default function App() {
  return (
    <ConfigProvider theme={{ algorithm: theme.darkAlgorithm, token: { colorPrimary: '#667eea' } }}>
      <AntApp>
        <Navbar />
        <Suspense fallback={<PageLoader />}>
          <Routes>
            <Route path="/" element={<HomePage />} />
            <Route path="/login" element={<LoginPage />} />
            <Route path="/register" element={<RegisterPage />} />
            <Route path="/products" element={<AuthGuard><ProductsPage /></AuthGuard>} />
            <Route path="/orders" element={<AuthGuard><OrdersPage /></AuthGuard>} />
            <Route path="/rank" element={<AuthGuard><RankPage /></AuthGuard>} />
          </Routes>
        </Suspense>
      </AntApp>
    </ConfigProvider>
  )
}
