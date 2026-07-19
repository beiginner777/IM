import { useEffect, useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { Table, Button, Empty } from 'antd'
import { ArrowLeftOutlined } from '@ant-design/icons'
import { useProductStore } from '../store/product'
import { useAuthStore } from '../store/auth'

export default function OrdersPage() {
  const nav = useNavigate()
  const { orders, fetchOrders } = useProductStore()
  const { username } = useAuthStore()
  const [loading, setLoading] = useState(false)

  useEffect(() => {
    if (!username) return
    setLoading(true)
    fetchOrders(username).finally(() => setLoading(false))
  }, [])

  return (
    <div style={{ minHeight: 'calc(100vh - 56px)', background: 'linear-gradient(160deg,#0f0c29,#1a1a3e 30%,#24243e 60%,#1a1a3e)', padding: 40, color: '#fff' }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: 20, marginBottom: 32 }}>
        <Button type="text" icon={<ArrowLeftOutlined />} onClick={() => nav('/products')} style={{ color: 'rgba(255,255,255,0.5)' }}>返回商城</Button>
        <h1 style={{ fontSize: 24, margin: 0 }}>我的抢购记录</h1>
      </div>
      {orders.length === 0 && !loading ? (
        <Empty description="暂无抢购记录"><Button type="primary" onClick={() => nav('/products')}>去抢购</Button></Empty>
      ) : (
        <Table dataSource={orders} loading={loading} rowKey="orderId" pagination={{ pageSize: 10 }}
          columns={[
            { title: '订单编号', dataIndex: 'orderId', width: 120 },
            { title: '商品名称', dataIndex: 'productName' },
            { title: '购买时间', dataIndex: 'time', width: 200 },
            { title: '状态', dataIndex: 'status', width: 100, render: (s: string) => <span style={{ color: s === '成功' ? '#52c41a' : '#f5222d' }}>{s}</span> },
          ]} />
      )}
    </div>
  )
}
