<script setup lang="ts">
import { onMounted, onUnmounted, ref } from 'vue'
import { Table, Tag } from 'ant-design-vue'
import { TrophyOutlined } from '@ant-design/icons-vue'
import { useProductStore } from '../store/product'
import type { RankItem } from '../api'

const productStore = useProductStore()

let timer: ReturnType<typeof setInterval> | null = null

// 记录上一次排行数据用于对比高亮
const prevRankMap = ref<Map<number, number>>(new Map())
const changedIds = ref<Set<number>>(new Set())

function startPolling() {
  productStore.fetchRank()
  timer = setInterval(async () => {
    // 保存旧排行位置
    const oldMap = new Map<number, number>()
    productStore.rankList.forEach((item, index) => {
      oldMap.set(item.productId, index)
    })
    prevRankMap.value = oldMap

    await productStore.fetchRank()

    // 对比找出变化的条目
    const changed = new Set<number>()
    productStore.rankList.forEach((item, index) => {
      const oldIndex = oldMap.get(item.productId)
      if (oldIndex === undefined || oldIndex !== index) {
        changed.add(item.productId)
      }
    })
    changedIds.value = changed

    // 2 秒后清除高亮
    setTimeout(() => {
      changedIds.value = new Set()
    }, 2000)
  }, 3000)
}

function stopPolling() {
  if (timer) {
    clearInterval(timer)
    timer = null
  }
}

const columns = [
  {
    title: '排名',
    key: 'rank',
    width: 80,
    dataIndex: 'rank',
  },
  {
    title: '商品名称',
    dataIndex: 'productName',
    key: 'productName',
  },
  {
    title: '购买次数',
    dataIndex: 'count',
    key: 'count',
    width: 120,
  },
]

function getRowClassName(record: RankItem) {
  return changedIds.value.has(record.productId) ? 'rank-row-changed' : ''
}

onMounted(() => {
  startPolling()
})

onUnmounted(() => {
  stopPolling()
})
</script>

<template>
  <div class="rank-list">
    <div class="rank-header">
      <TrophyOutlined class="rank-icon" />
      <span class="rank-title">实时排行榜</span>
      <span class="rank-subtitle">每 3 秒自动刷新</span>
    </div>

    <Table
      :columns="columns"
      :dataSource="productStore.rankList"
      :pagination="false"
      :loading="false"
      rowKey="productId"
      size="middle"
      :rowClassName="getRowClassName"
    >
      <template #bodyCell="{ column, index, text }">
        <template v-if="column.key === 'rank'">
          <span v-if="index < 3" style="font-size: 20px">
            {{ index === 0 ? '🥇' : index === 1 ? '🥈' : '🥉' }}
          </span>
          <span v-else class="rank-number">{{ index + 1 }}</span>
        </template>
        <template v-else-if="column.key === 'count'">
          <Tag color="blue" style="font-size: 14px; font-weight: 600">
            {{ text }} 次
          </Tag>
        </template>
      </template>
    </Table>
  </div>
</template>

<style scoped>
.rank-list {
  background: #fff;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.rank-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 16px;
}

.rank-icon {
  font-size: 22px;
  color: #faad14;
}

.rank-title {
  font-size: 18px;
  font-weight: 700;
  color: #1a1a2e;
}

.rank-subtitle {
  font-size: 12px;
  color: #bbb;
  margin-left: 4px;
}

.rank-number {
  color: #8c8c8c;
  font-weight: 500;
}

:deep(.rank-row-changed) {
  animation: highlightRow 2s ease-out;
}

@keyframes highlightRow {
  0% {
    background-color: #fff7e6;
  }
  100% {
    background-color: transparent;
  }
}
</style>
