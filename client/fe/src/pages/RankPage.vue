<script setup lang="ts">
import { onMounted, onUnmounted, ref } from 'vue'
import { Empty, Spin } from 'ant-design-vue'
import { TrophyOutlined, FireOutlined, ArrowLeftOutlined } from '@ant-design/icons-vue'
import { useRouter } from 'vue-router'
import { useProductStore } from '../store/product'

const router = useRouter()
const productStore = useProductStore()
const loading = ref(false)
let timer: number | null = null

onMounted(async () => {
  loading.value = true
  await productStore.fetchRank()
  loading.value = false
  timer = window.setInterval(async () => {
    await productStore.fetchRank()
  }, 3000)
})

onUnmounted(() => {
  if (timer) clearInterval(timer)
})
</script>

<template>
  <div class="rank-page">
    <header class="page-header">
      <span class="back-btn" @click="router.push('/products')">
        <ArrowLeftOutlined /> 返回商城
      </span>
      <h1><TrophyOutlined /> 抢购排行榜</h1>
    </header>
    <main>
      <Spin :spinning="loading">
        <Empty v-if="productStore.rankList.length === 0" description="暂无抢购数据" />
        <ul v-else class="rank-list">
          <li v-for="(item, idx) in productStore.rankList" :key="item.productId" class="rank-item" :class="{ 'top3': idx < 3 }">
            <span class="rank-idx">
              <FireOutlined v-if="idx < 3" />
              {{ idx + 1 }}
            </span>
            <span class="rank-name">{{ item.productName }}</span>
            <span class="rank-count">{{ item.count }} 件</span>
          </li>
        </ul>
      </Spin>
    </main>
  </div>
</template>

<style scoped>
.rank-page {
  min-height: calc(100vh - 56px);
  background: linear-gradient(160deg, #0f0c29 0%, #1a1a3e 30%, #24243e 60%, #1a1a3e 100%);
  padding: 40px; color: #fff;
}
.page-header {
  display: flex; align-items: center; gap: 20px; margin-bottom: 32px;
}
.back-btn { cursor: pointer; color: rgba(255,255,255,0.5); font-size: 14px; }
.back-btn:hover { color: #667eea; }
.page-header h1 { font-size: 24px; margin: 0; }
.rank-item {
  display: flex; align-items: center; gap: 16px; padding: 16px 20px;
  background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.05);
  border-radius: 12px; margin-bottom: 8px; max-width: 600px;
}
.rank-item.top3 { border-color: rgba(250,173,20,0.25); background: rgba(250,173,20,0.06); }
.rank-idx { font-size: 18px; font-weight: 700; width: 40px; text-align: center; }
.rank-name { flex: 1; font-size: 16px; }
.rank-count { color: #f5222d; font-weight: 700; }
</style>
