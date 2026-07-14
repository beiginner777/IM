import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { Product, RankItem, Order } from '../api'
import { getProducts, getRank, getOrders } from '../api'

export const useProductStore = defineStore('product', () => {
  const products = ref<Product[]>([])
  const rankList = ref<RankItem[]>([])
  const orders = ref<Order[]>([])
  const loading = ref(false)

  async function fetchProducts() {
    loading.value = true
    try {
      const res = await getProducts()
      products.value = res.data
    } finally {
      loading.value = false
    }
  }

  /** 购买成功后局部更新库存 */
  function decrementStock(productId: number) {
    const product = products.value.find((p) => p.id === productId)
    if (product && product.stock > 0) {
      product.stock--
    }
  }

  async function fetchRank() {
    try {
      const res = await getRank()
      rankList.value = res.data
    } catch {
      // 轮询静默失败，不弹错误提示
    }
  }

  async function fetchOrders(uid: string) {
    try {
      const res = await getOrders(uid)
      orders.value = res.data
    } catch {
      // 错误已在拦截器中处理
    }
  }

  return {
    products,
    rankList,
    orders,
    loading,
    fetchProducts,
    decrementStock,
    fetchRank,
    fetchOrders,
  }
})
