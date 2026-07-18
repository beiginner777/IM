import { useEffect } from 'react'
import { Spin, Empty } from 'antd'
import { useProductStore } from '../store/product'
import ProductCard from '../components/ProductCard'
import RankList from '../components/RankList'

export default function ProductListPage() {
  const { products, loading, fetchProducts } = useProductStore()

  useEffect(() => { fetchProducts() }, [])

  return (
    <div style={{ minHeight: 'calc(100vh - 56px)', background: '#f0f2f5' }}>
      <div style={{ maxWidth: 1400, margin: '0 auto', padding: 24, display: 'flex', gap: 24 }}>
        <div style={{ flex: 1 }}>
          <h2 style={{ marginBottom: 20, fontSize: 22 }}>🔥 限时秒杀</h2>
          <Spin spinning={loading}>
            {!loading && products.length === 0 ? <Empty description="暂无商品" /> : (
              <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(260px, 1fr))', gap: 20 }}>
                {products.map(p => <ProductCard key={p.id} product={p} />)}
              </div>
            )}
          </Spin>
        </div>
        <aside style={{ width: 300, flexShrink: 0 }}><RankList /></aside>
      </div>
    </div>
  )
}
