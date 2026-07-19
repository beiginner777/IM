import { useEffect, useRef } from 'react'
import { Empty, Typography } from 'antd'
import { TrophyOutlined, FireOutlined } from '@ant-design/icons'
import { useProductStore } from '../store/product'

export default function RankList() {
  const { rankList, fetchRank } = useProductStore()
  const timer = useRef<number>()

  useEffect(() => {
    fetchRank()
    timer.current = window.setInterval(fetchRank, 3000)
    return () => clearInterval(timer.current)
  }, [])

  return (
    <div style={{ background: 'rgba(255,255,255,0.03)', border: '1px solid rgba(255,255,255,0.06)', borderRadius: 14, padding: 20, color: '#fff' }}>
      <Typography.Title level={5} style={{ color: '#e0e0e0', margin: '0 0 16px' }}><TrophyOutlined /> 抢购排行</Typography.Title>
      {rankList.length === 0 ? <Empty description="暂无数据" image={Empty.PRESENTED_IMAGE_SIMPLE} /> : (
        <ul style={{ listStyle: 'none', padding: 0, margin: 0 }}>
          {rankList.map((item, idx) => (
            <li key={item.productId} style={{ display: 'flex', alignItems: 'center', gap: 10, padding: '8px 0', borderBottom: '1px solid rgba(255,255,255,0.04)', fontSize: 14 }}>
              <span style={{ width: 28, textAlign: 'center', color: idx < 3 ? '#faad14' : 'rgba(255,255,255,0.4)' }}>{idx < 3 && <FireOutlined />} {idx + 1}</span>
              <span style={{ flex: 1, color: 'rgba(255,255,255,0.7)' }}>{item.productName}</span>
              <span style={{ color: 'rgba(255,255,255,0.4)' }}>{item.count} 件</span>
            </li>
          ))}
        </ul>
      )}
    </div>
  )
}
