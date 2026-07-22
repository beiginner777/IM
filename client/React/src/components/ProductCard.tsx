import { useState, useRef } from 'react'
import { Card, Button, Tag, Modal, Input, message } from 'antd'
import { ThunderboltOutlined } from '@ant-design/icons'
import type { Product } from '../api'
import { buyProduct } from '../api'
import { useProductStore } from '../store/product'

export default function ProductCard({ product }: { product: Product }) {
  const [loading, setLoading] = useState(false)
  const [pwdModal, setPwdModal] = useState(false)
  const pwdRef = useRef<string>('')
  const decrementStock = useProductStore(s => s.decrementStock)

  const handleBuy = () => { setPwdModal(true) }

  const doBuy = async () => {
    if (loading) return
    setLoading(true)
    try {
      const res = await buyProduct(product.id, pwdRef.current)
      if (res.data.success) {
        decrementStock(product.id)
        setPwdModal(false)
        Modal.success({ title: '抢购成功！', content: `恭喜抢到 ${product.name}！` })
      } else {
        message.error(res.data.message || '抢购失败')
      }
    } catch {} finally { setLoading(false) }
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
      <Modal title="确认购买" open={pwdModal} onOk={doBuy} onCancel={()=>setPwdModal(false)} okText="确认购买" cancelText="取消">
        <div style={{ marginBottom:8 }}>商品: {product.name}  ¥{product.price}</div>
        <Input.Password placeholder="请输入登录密码" onChange={e=>pwdRef.current=e.target.value} />
      </Modal>
    </Card>
  )
}
