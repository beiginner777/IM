import { Routes, Route } from 'react-router-dom'
import Navbar from './components/Navbar'
import AuthGuard from './components/AuthGuard'
import HomePage from './pages/HomePage'
import LoginPage from './pages/LoginPage'
import RegisterPage from './pages/RegisterPage'
import ProductsPage from './pages/ProductListPage'
import OrdersPage from './pages/OrdersPage'
import RankPage from './pages/RankPage'
import RechargePage from './pages/RechargePage'

export default function App() {
  return (
    <>
      <Navbar />
      <Routes>
        <Route path="/" element={<HomePage />} />
        <Route path="/login" element={<LoginPage />} />
        <Route path="/register" element={<RegisterPage />} />
        <Route path="/products" element={<AuthGuard><ProductsPage /></AuthGuard>} />
        <Route path="/orders" element={<AuthGuard><OrdersPage /></AuthGuard>} />
        <Route path="/rank" element={<AuthGuard><RankPage /></AuthGuard>} />
        <Route path="/recharge" element={<AuthGuard><RechargePage /></AuthGuard>} />
      </Routes>
    </>
  )
}
