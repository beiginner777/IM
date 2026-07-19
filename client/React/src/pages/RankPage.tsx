import { useEffect, useRef } from 'react'
import { useNavigate } from 'react-router-dom'
import { Button, Empty } from 'antd'
import { ArrowLeftOutlined, TrophyOutlined, FireOutlined } from '@ant-design/icons'
import { useProductStore } from '../store/product'

export default function RankPage() {
  const nav = useNavigate()
  const { rankList, fetchRank } = useProductStore()
  const timer = useRef<number>()

  useEffect(() => {
    fetchRank()
    timer.current = window.setInterval(fetchRank, 3000)
    return () => clearInterval(timer.current)
  }, [])

  return (
    <div style={{ minHeight: 'calc(100vh - 56px)', background: 'linear-gradient(160deg,#0f0c29,#1a1a3e 30%,#24243e 60%,#1a1a3e)', padding: 40, color: '#fff' }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 20, marginBottom: 32 }}>
        <Button type="text" icon={<ArrowLeftOutlined />} onClick={() => nav('/products')} style={{ color: 'rgba(255,255,255,0.5)' }}>返回商城</Button>
        <h1 style={{ fontSize: 24, margin: 0 }}><TrophyOutlined /> 抢购排行榜</h1>
      </div>
      {rankList.length === 0 ? <Empty description="暂无抢购数据" /> : (
        <ul style={{ listStyle: 'none', padding: 0, maxWidth: 600 }}>
          {rankList.map((item, idx) => (
            <li key={item.productId} style={{
              display: 'flex', alignItems: 'center', gap: 16, padding: '16px 20px',
              background: 'rgba(255,255,255,0.03)', border: `1px solid ${idx < 3 ? 'rgba(250,173,20,0.25)' : 'rgba(255,255,255,0.05)'}`,
              borderRadius: 12, marginBottom: 8,
            }}>
              <span style={{ fontSize: 18, fontWeight: 700, width: 40, textAlign: 'center' }}>{idx < 3 && <FireOutlined style={{ color: '#faad14' }} />} {idx + 1}</span>
              <span style={{ flex: 1, fontSize: 16 }}>{item.productName}</span>
              <span style={{ color: '#f5222d', fontWeight: 700 }}>{item.count} 件</span>
            </li>
          ))}
        </ul>
      )}
    </div>
  )
}
