import { useEffect, useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { Card, List, Spin, Typography } from 'antd'
import { UserOutlined, WalletOutlined, ShoppingOutlined } from '@ant-design/icons'
import request from '../api/request'

export default function ProfilePage() {
  const [data, setData] = useState<any>(null)
  const nav = useNavigate()

  useEffect(() => {
    request.get('/profile').then(r => setData(r.data)).catch(() => nav('/login'))
  }, [])

  if (!data) return <div style={{ display:'flex',justifyContent:'center',alignItems:'center',minHeight:'100vh',background:'#0a0a1a' }}><Spin size="large" /></div>

  return (
    <div style={{ minHeight:'100vh',background:'#0a0a1a',padding:'40px',display:'flex',justifyContent:'center' }}>
      <div style={{ width:700 }}>
        <Card style={{ background:'rgba(255,255,255,0.04)',border:'1px solid rgba(255,255,255,0.08)',borderRadius:16,marginBottom:24 }}>
          <div style={{ display:'flex',gap:24,alignItems:'center' }}>
            <div style={{ width:64,height:64,borderRadius:32,background:'linear-gradient(135deg,#667eea,#764ba2)',display:'flex',alignItems:'center',justifyContent:'center',fontSize:28,color:'#fff' }}>
              <UserOutlined />
            </div>
            <div>
              <Typography.Title level={3} style={{ color:'#fff',margin:0 }}>UID: {data.uid}</Typography.Title>
              <div style={{ color:'#ffd700',fontSize:20,marginTop:8 }}><WalletOutlined /> 余额 ¥{data.balance?.toFixed(2)}</div>
            </div>
          </div>
        </Card>
        <Card title={<span style={{ color:'#fff' }}><ShoppingOutlined /> 购买的宝贝</span>}
          style={{ background:'rgba(255,255,255,0.04)',border:'1px solid rgba(255,255,255,0.08)',borderRadius:16 }}
          headStyle={{ color:'#fff',borderColor:'rgba(255,255,255,0.08)' }}>
          <List dataSource={data.orders||[]} locale={{ emptyText:'暂无购买记录' }}
            renderItem={(o:any) => (
              <List.Item style={{ borderColor:'rgba(255,255,255,0.06)' }}>
                <span style={{ color:'#fff' }}>{o.productName}</span>
                <span style={{ color:'#ffd700' }}>¥{o.price}</span>
                <span style={{ color:'rgba(255,255,255,0.35)' }}>{o.time}</span>
              </List.Item>
            )} />
        </Card>
      </div>
    </div>
  )
}
