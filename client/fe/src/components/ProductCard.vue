<script setup lang="ts">
import { ref } from 'vue'
import { Card, Button, Tag, message, Modal } from 'ant-design-vue'
import { ThunderboltOutlined } from '@ant-design/icons-vue'
import type { Product } from '../api'
import { buyProduct } from '../api'
import { useProductStore } from '../store/product'

const props = defineProps<{ product: Product }>()
const productStore = useProductStore()
const loading = ref(false)

async function handleBuy() {
  if (loading.value) return
  loading.value = true
  try {
    const res = await buyProduct(props.product.id)
    if (res.data.success) {
      productStore.decrementStock(props.product.id)
      Modal.success({
        title: '抢购成功！',
        content: `恭喜你成功抢到 ${props.product.name}！`,
        okText: '好的',
      })
    } else {
      message.error(res.data.message || '抢购失败')
    }
  } catch {
    // 错误已在拦截器中处理
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <Card hoverable class="product-card" :bodyStyle="{ padding: '16px' }">
    <div class="card-img">
      <img :src="product.imageUrl" :alt="product.name" />
      <Tag v-if="product.stock <= 10 && product.stock > 0" color="red" class="stock-tag">
        仅剩 {{ product.stock }}
      </Tag>
      <Tag v-if="product.stock === 0" color="default" class="stock-tag">已售罄</Tag>
    </div>
    <div class="card-body">
      <h3 class="card-title">{{ product.name }}</h3>
      <div class="card-price">
        <span class="price-symbol">¥</span>
        <span class="price-num">{{ product.price }}</span>
      </div>
      <Button
        type="primary"
        block
        size="large"
        :loading="loading"
        :disabled="product.stock === 0"
        @click="handleBuy"
        class="buy-btn"
      >
        <template #icon><ThunderboltOutlined /></template>
        {{ product.stock === 0 ? '已售罄' : '立即抢购' }}
      </Button>
    </div>
  </Card>
</template>

<style scoped>
.product-card {
  background: rgba(255,255,255,0.05) !important;
  border: 1px solid rgba(255,255,255,0.08) !important;
  border-radius: 16px !important;
  overflow: hidden;
  transition: all 0.3s ease;
}
.product-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 8px 24px rgba(0,0,0,0.3);
  border-color: rgba(102,126,234,0.3) !important;
}
.card-img {
  position: relative; overflow: hidden; border-radius: 10px; margin-bottom: 12px;
  aspect-ratio: 4/3; background: #1a1a2e;
}
.card-img img { width: 100%; height: 100%; object-fit: cover; }
.stock-tag { position: absolute; top: 8px; right: 8px; }
.card-title { color: #e0e0e0; font-size: 15px; margin: 0 0 8px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.card-price { margin-bottom: 12px; color: #f5222d; }
.price-symbol { font-size: 16px; font-weight: 600; }
.price-num { font-size: 24px; font-weight: 800; }
.buy-btn {
  background: linear-gradient(135deg, #667eea, #764ba2) !important;
  border: none !important; font-weight: 700; border-radius: 10px !important;
}
</style>
