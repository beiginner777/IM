import { useEffect, useState } from 'react'
import { Button, InputNumber, message, Card, Typography } from 'antd'
import { WalletOutlined, PlusOutlined } from '@ant-design/icons'
import request from '../api/request'

export default function RechargePage() {
  const [balance, setBalance] = useState(0)
  const [amount, setAmount] = useState(1000)
  const [loading, setLoading] = useState(false)

  const fetchBalance = async () => {
    try {
      const res = await request.get('/recharge')
      setBalance(res.data.balance)
    } catch { /* ignore */ }
  }

  useEffect(() => { fetchBalance() }, [])

  const handleRecharge = async () => {
    if (amount <= 0) { message.warning('请输入充值金额'); return }
    setLoading(true)
    try {
      const res = await request.post('/recharge', { amount })
      setBalance(res.data.balance)
      message.success(`充值成功！当前余额 ¥${res.data.balance}`)
    } catch {} finally { setLoading(false) }
  }

  return (
    <div style={{ minHeight: 'calc(100vh - 56px)', background: 'linear-gradient(160deg, #0f0c29, #1a1a3e 30%, #24243e 60%, #1a1a3e)', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
      <Card style={{ width: 420, background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.08)', borderRadius: 20, backdropFilter: 'blur(24px)' }} bodyStyle={{ padding: '40px' }}>
        <div style={{ textAlign: 'center', marginBottom: 32 }}>
          <div style={{ display: 'inline-flex', width: 56, height: 56, borderRadius: 16, background: 'linear-gradient(135deg,#667eea,#764ba2)', alignItems: 'center', justifyContent: 'center', fontSize: 28, color: '#fff', marginBottom: 16 }}>
            <WalletOutlined />
          </div>
          <Typography.Title level={3} style={{ color: '#fff', margin: '0 0 8px', letterSpacing: 2 }}>账户充值</Typography.Title>
          <Typography.Text style={{ color: 'rgba(255,255,255,0.45)' }}>充值后即可参与限时秒杀</Typography.Text>
        </div>

        <div style={{ textAlign: 'center', marginBottom: 28, padding: '20px', background: 'rgba(255,255,255,0.04)', borderRadius: 12 }}>
          <Typography.Text style={{ color: 'rgba(255,255,255,0.4)', fontSize: 13 }}>当前余额</Typography.Text>
          <div style={{ fontSize: 40, fontWeight: 800, color: '#52c41a', marginTop: 4 }}>¥{balance.toLocaleString()}</div>
        </div>

        <div style={{ display: 'flex', gap: 12, marginBottom: 8 }}>
          <InputNumber
            size="large"
            style={{ flex: 1 }}
            min={100}
            max={100000}
            step={500}
            value={amount}
            onChange={v => setAmount(v || 0)}
            formatter={v => `¥ ${v}`.replace(/\B(?=(\d{3})+(?!\d))/g, ',')}
            parser={v => Number((v || '').replace(/[^\d]/g, '')) as any}
          />
          <Button type="primary" size="large" icon={<PlusOutlined />} loading={loading} onClick={handleRecharge}
            style={{ background: 'linear-gradient(135deg,#667eea,#764ba2)', border: 'none', fontWeight: 700, borderRadius: 10 }}>
            充值
          </Button>
        </div>

        <div style={{ display: 'flex', gap: 8, justifyContent: 'center', marginTop: 8 }}>
          {[1000, 2000, 5000, 10000].map(v => (
            <Button key={v} size="small" type={amount === v ? 'primary' : 'default'} ghost={amount !== v}
              onClick={() => setAmount(v)} style={{ borderRadius: 8 }}>¥{v}</Button>
          ))}
        </div>
      </Card>
    </div>
  )
}
