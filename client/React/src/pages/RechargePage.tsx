import { useState } from 'react'
import { Form, InputNumber, Input, Button, message } from 'antd'
import { useAuthStore } from '../store/auth'
import request from '../api/request'

export default function RechargePage() {
  const { balance, setBalance } = useAuthStore()
  const [loading, setLoading] = useState(false)

  const onFinish = async (v: { amount: number; password: string }) => {
    if (!v.password) { message.error('请输入密码'); return }
    setLoading(true)
    try {
      const res = await request.post('/recharge', { amount: v.amount, password: v.password })
      if (res.data.code === 0) {
        setBalance(res.data.balance)
        message.success(`充值成功！余额 ¥${res.data.balance.toFixed(2)}`)
      } else {
        message.error(res.data.message || '充值失败')
      }
    } catch { message.error('网络错误') }
    finally { setLoading(false) }
  }

  return (
    <div style={{ display: 'flex', justifyContent: 'center', alignItems: 'center', minHeight: '100vh', background: '#0a0a1a' }}>
      <div style={{ width: 420, padding: 40, background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.08)', borderRadius: 16 }}>
        <h1 style={{ color: '#fff', textAlign: 'center', marginBottom: 24 }}>账户充值</h1>
        <div style={{ textAlign: 'center', color: '#aaa', marginBottom: 24 }}>
          当前余额：<span style={{ color: '#ffd700', fontSize: 28, fontWeight: 'bold' }}>¥ {balance.toFixed(2)}</span>
        </div>
        <Form onFinish={onFinish} layout="vertical" size="large" initialValues={{ amount: 100 }}>
          <Form.Item label="充值金额" name="amount" rules={[{ required: true }]}>
            <InputNumber min={1} precision={2} style={{ width: '100%' }} placeholder="请输入金额" />
          </Form.Item>
          <Form.Item label="登录密码" name="password" rules={[{ required: true, message: '请输入密码确认' }]}>
            <Input.Password placeholder="请输入密码确认" />
          </Form.Item>
          <Form.Item>
            <Button type="primary" htmlType="submit" loading={loading} block>确认充值</Button>
          </Form.Item>
        </Form>
      </div>
    </div>
  )
}
