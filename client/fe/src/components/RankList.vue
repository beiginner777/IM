<script setup lang="ts">
import { onMounted, onUnmounted, ref } from 'vue'
import { Empty, Spin } from 'ant-design-vue'
import { TrophyOutlined, FireOutlined } from '@ant-design/icons-vue'
import { useProductStore } from '../store/product'

const productStore = useProductStore()
const loading = ref(false)
let timer: number | null = null

onMounted(async () => {
  loading.value = true
  await productStore.fetchRank()
  loading.value = false
  // 每 3 秒轮询排行榜
  timer = window.setInterval(async () => {
    await productStore.fetchRank()
  }, 3000)
})

onUnmounted(() => {
  if (timer) clearInterval(timer)
})
</script>

<template>
  <div class="rank-widget">
    <h3 class="rank-title"><TrophyOutlined /> 抢购排行</h3>
    <Spin :spinning="loading">
      <Empty v-if="productStore.rankList.length === 0" description="暂无数据" :image="Empty.PRESENTED_IMAGE_SIMPLE" />
      <ul v-else class="rank-list">
        <li v-for="(item, idx) in productStore.rankList" :key="item.productId" class="rank-item">
          <span class="rank-idx" :class="{ 'top3': idx < 3 }">
            <FireOutlined v-if="idx < 3" />
            {{ idx + 1 }}
          </span>
          <span class="rank-name">{{ item.productName }}</span>
          <span class="rank-count">{{ item.count }} 件</span>
        </li>
      </ul>
    </Spin>
  </div>
</template>

<style scoped>
.rank-widget {
  background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.06);
  border-radius: 14px; padding: 20px; color: #fff;
}
.rank-title { font-size: 16px; margin: 0 0 16px; color: #e0e0e0; }
.rank-list { list-style: none; padding: 0; margin: 0; }
.rank-item {
  display: flex; align-items: center; gap: 10px;
  padding: 8px 0; border-bottom: 1px solid rgba(255,255,255,0.04); font-size: 14px;
}
.rank-idx { width: 28px; text-align: center; color: rgba(255,255,255,0.4); }
.rank-idx.top3 { color: #faad14; }
.rank-name { flex: 1; color: rgba(255,255,255,0.7); }
.rank-count { color: rgba(255,255,255,0.4); }
</style>
