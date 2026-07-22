import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { Card, Button, Tag, message } from 'antd'
import { ThunderboltOutlined } from '@ant-design/icons'
import type { Product } from '../api'
import { buyProduct } from '../api'

export default function ProductCard({ product }: { product: Product }) {
  const [loading, setLoading] = useState(false)
  const nav = useNavigate()

  const handleBuy = async () => {
    if (loading) return
    setLoading(true)
    try {
      const res = await buyProduct(product.id, '')
      if (res.data.success && res.data.orderId) {
        nav('/order/' + res.data.orderId)
      } else {
        message.error(res.data.message || '创建订单失败')
      }
    } catch { message.error('网络错误') } finally { setLoading(false) }
  }

  return (
    <Card hoverable style={{ background: 'rgba(255,255,255,0.05)', border: '1px solid rgba(255,255,255,0.08)', borderRadius: 16, overflow: 'hidden' }}
      bodyStyle={{ padding: 16 }}>
      <div style={{ position: 'relative', borderRadius: 10, overflow: 'hidden', marginBottom: 12, aspectRatio: '4/3', background: '#1a1a2e' }}>
        <img src={product.imageUrl} alt={product.name} style={{ width: '100%', height: '100%', objectFit: 'cover' }} />
        {product.stock > 0 && product.stock <= 10 && <Tag color="red" style={{ position: 'absolute', top: 8, right: 8 }}>仅剩 {product.stock}</Tag>}
        {product.stock === 0 && <Tag style={{ position: 'absolute', top: 8, right: 8 }}>已售罄</Tag>}
      </div>
      <h3 style={{ color: '#e0e0e0', fontSize: 15, margin: '0 0 8px', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{product.name}</h3>
      <div style={{ marginBottom: 12, color: '#f5222d' }}><span style={{ fontSize: 16, fontWeight: 600 }}>¥</span><span style={{ fontSize: 24, fontWeight: 800 }}>{product.price}</span></div>
      <Button type="primary" block size="large" loading={loading} disabled={product.stock === 0} icon={<ThunderboltOutlined />} onClick={handleBuy}
        style={{ background: 'linear-gradient(135deg,#667eea,#764ba2)', border: 'none', fontWeight: 700, borderRadius: 10 }}>
        {product.stock === 0 ? '已售罄' : '立即抢购'}
      </Button>
    </Card>
  )
}
