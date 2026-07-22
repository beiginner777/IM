import { useNavigate, Link } from 'react-router-dom'
import { Button, Space } from 'antd'
import { ThunderboltOutlined, UserOutlined, WalletOutlined } from '@ant-design/icons'
import { useAuthStore } from '../store/auth'

export default function Navbar() {
  const { isLoggedIn, username, logout } = useAuthStore()
  const nav = useNavigate()

  return (
    <header style={{
      position: 'sticky', top: 0, zIndex: 1000, display: 'flex', alignItems: 'center',
      justifyContent: 'space-between', padding: '0 32px', height: 56,
      background: 'rgba(15,12,41,0.85)', backdropFilter: 'blur(16px)',
      borderBottom: '1px solid rgba(255,255,255,0.06)'
    }}>
      <Link to="/" style={{ display: 'flex', alignItems: 'center', gap: 10, color: '#fff', textDecoration: 'none', fontSize: 18, fontWeight: 800, letterSpacing: 2 }}>
        <span style={{ display: 'inline-flex', alignItems: 'center', justifyContent: 'center', width: 32, height: 32, borderRadius: 8, background: 'linear-gradient(135deg,#667eea,#764ba2)', fontSize: 16 }}><ThunderboltOutlined /></span>
        秒杀商城
      </Link>
      <Space>
        {isLoggedIn ? (
          <>
            <span style={{ color: 'rgba(255,255,255,0.5)', fontSize: 14 }}><UserOutlined /> {username}</span>
            <Button type="text" style={{ color: 'rgba(255,255,255,0.7)' }} onClick={() => nav('/products')}>商品</Button>
            <Button type="text" style={{ color: 'rgba(255,255,255,0.7)' }} onClick={() => nav('/orders')}>订单</Button>
            <Button type="text" style={{ color: 'rgba(255,255,255,0.7)' }} onClick={() => nav('/rank')}>排行</Button>
            <Button type="text" style={{ color: '#52c41a' }} icon={<WalletOutlined />} onClick={() => nav('/recharge')}>充值</Button>
            <Button type="text" danger onClick={() => { logout(); nav('/login') }}>退出</Button>
          </>
        ) : (
          <>
            <Button type="primary" ghost size="small" onClick={() => nav('/login')}>登 录</Button>
            <Button type="text" size="small" style={{ color: 'rgba(255,255,255,0.7)' }} onClick={() => nav('/register')}>注 册</Button>
          </>
        )}
      </Space>
    </header>
  )
}
