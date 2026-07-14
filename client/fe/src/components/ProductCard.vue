<script setup lang="ts">
import { ref } from 'vue'
import { Card, Button, Tag, message } from 'ant-design-vue'
import { ShoppingCartOutlined } from '@ant-design/icons-vue'
import type { Product } from '../api'
import { buyProduct } from '../api'
import { useProductStore } from '../store/product'

const props = defineProps<{
  product: Product
}>()

const productStore = useProductStore()
const buying = ref(false)

async function handleBuy() {
  if (buying.value) return
  buying.value = true
  try {
    const res = await buyProduct(props.product.id)
    if (res.data.success) {
      message.success('抢购成功！')
      productStore.decrementStock(props.product.id)
    } else {
      message.error(res.data.message || '抢购失败')
    }
  } catch {
    // 错误已在拦截器中处理
  } finally {
    buying.value = false
  }
}
</script>

<template>
  <Card class="product-card" hoverable :bordered="false">
    <template #cover>
      <div class="product-image-wrapper">
        <img
          :src="product.imageUrl || `https://picsum.photos/seed/${product.id}/400/240`"
          :alt="product.name"
          class="product-image"
        />
        <Tag v-if="product.stock === 0" color="red" class="sold-out-tag">
          已售罄
        </Tag>
      </div>
    </template>

    <Card.Meta>
      <template #title>
        <div class="product-name">{{ product.name }}</div>
      </template>
      <template #description>
        <div class="product-info">
          <div class="product-price">
            <span class="price-symbol">¥</span>
            <span class="price-value">{{ product.price.toFixed(2) }}</span>
          </div>
          <div class="product-stock">
            库存：<span :class="{ 'low-stock': product.stock > 0 && product.stock <= 10 }">
              {{ product.stock }}
            </span>
          </div>
        </div>
      </template>
    </Card.Meta>

    <Button
      type="primary"
      block
      size="large"
      :disabled="product.stock === 0"
      :loading="buying"
      @click="handleBuy"
      class="buy-button"
    >
      <template #icon>
        <ShoppingCartOutlined />
      </template>
      {{ product.stock === 0 ? '已售罄' : '立即抢购' }}
    </Button>
  </Card>
</template>

<style scoped>
.product-card {
  border-radius: 12px;
  overflow: hidden;
  transition: transform 0.2s, box-shadow 0.2s;
}

.product-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12);
}

.product-image-wrapper {
  position: relative;
  height: 200px;
  overflow: hidden;
  background: #f5f5f5;
}

.product-image {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.sold-out-tag {
  position: absolute;
  top: 12px;
  right: 12px;
  font-size: 13px;
}

.product-name {
  font-size: 16px;
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.product-info {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.product-price {
  color: #ff4d4f;
  font-weight: 700;
}

.price-symbol {
  font-size: 14px;
}

.price-value {
  font-size: 22px;
}

.product-stock {
  color: #8c8c8c;
  font-size: 13px;
}

.low-stock {
  color: #faad14;
  font-weight: 600;
}

.buy-button {
  margin-top: 12px;
  height: 40px;
  font-size: 15px;
  font-weight: 600;
  border-radius: 8px;
}
</style>
