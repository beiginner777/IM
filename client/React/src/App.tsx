import { Routes, Route, Navigate } from 'react-router-dom'
import { ConfigProvider, theme, App as AntApp } from 'antd'
import Navbar from './components/Navbar'
import HomePage from './pages/HomePage'
import LoginPage from './pages/LoginPage'
import RegisterPage from './pages/RegisterPage'
import ProductsPage from './pages/ProductListPage'
import OrdersPage from './pages/OrdersPage'
import RankPage from './pages/RankPage'
import AuthGuard from './components/AuthGuard'

export default function App() {
  return (
    <ConfigProvider theme={{ algorithm: theme.darkAlgorithm, token: { colorPrimary: '#667eea' } }}>
      <AntApp>
        <Navbar />
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/login" element={<LoginPage />} />
          <Route path="/register" element={<RegisterPage />} />
          <Route path="/products" element={<AuthGuard><ProductsPage /></AuthGuard>} />
          <Route path="/orders" element={<AuthGuard><OrdersPage /></AuthGuard>} />
          <Route path="/rank" element={<AuthGuard><RankPage /></AuthGuard>} />
        </Routes>
      </AntApp>
    </ConfigProvider>
  )
}
