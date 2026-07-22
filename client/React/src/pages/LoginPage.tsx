import { useState } from 'react'
import { useNavigate, useSearchParams, Link } from 'react-router-dom'
import { Form, Input, Button, message, Typography } from 'antd'
import { UserOutlined, LockOutlined, ThunderboltOutlined } from '@ant-design/icons'
import { login, setBaseURL } from '../api'
import { useAuthStore } from '../store/auth'

export default function LoginPage() {
  const [loading, setLoading] = useState(false)
  const nav = useNavigate()
  const [params] = useSearchParams()
  const authStore = useAuthStore()

  const onFinish = async (v: { username: string; password: string }) => {
    setLoading(true)
    try {
      const res = await login(v)
      const { username, host, port } = res.data
      authStore.loginSuccess(username, host, port)
      setBaseURL(host, port)
      message.success(`欢迎 ${username}，登录成功！`, 1.5)
      nav(params.get('redirect') || '/products')
    } catch {} finally { setLoading(false) }
  }

  return (
    <div style={{ minHeight: 'calc(100vh - 56px)', background: 'linear-gradient(160deg, #0f0c29, #1a1a3e 30%, #24243e 60%, #1a1a3e)', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
      <div style={{ width: 420, padding: '48px 40px 40px', background: 'rgba(255,255,255,0.04)', backdropFilter: 'blur(24px)', border: '1px solid rgba(255,255,255,0.08)', borderRadius: 24 }}>
        <div style={{ textAlign: 'center', marginBottom: 36 }}>
          <div style={{ display: 'inline-flex', width: 56, height: 56, borderRadius: 16, background: 'linear-gradient(135deg,#667eea,#764ba2)', alignItems: 'center', justifyContent: 'center', fontSize: 28, color: '#fff', marginBottom: 16 }}><ThunderboltOutlined /></div>
          <h1 style={{ fontSize: 28, fontWeight: 800, color: '#fff', margin: '0 0 6px', letterSpacing: 2 }}>秒杀商城</h1>
          <Typography.Text style={{ color: 'rgba(255,255,255,0.45)' }}>登录 IM 账号，参与限时抢购</Typography.Text>
        </div>
        <Form onFinish={onFinish} size="large">
          <Form.Item name="username" rules={[{ required: true, message: '请输入用户名' }]}>
            <Input prefix={<UserOutlined />} placeholder="用户名" />
          </Form.Item>
          <Form.Item name="password" rules={[{ required: true, message: '请输入密码' }]}>
            <Input.Password prefix={<LockOutlined />} placeholder="密码" />
          </Form.Item>
          <Form.Item>
            <Button type="primary" htmlType="submit" loading={loading} block style={{ height: 48, borderRadius: 12, background: 'linear-gradient(135deg,#667eea,#764ba2)', border: 'none', fontWeight: 700, letterSpacing: 4 }}>登 录</Button>
          </Form.Item>
        </Form>
        <div style={{ textAlign: 'center', color: 'rgba(255,255,255,0.4)' }}>还没有账号？<Link to="/register" style={{ color: '#667eea', fontWeight: 600 }}>立即注册 →</Link></div>
      </div>
    </div>
  )
}
