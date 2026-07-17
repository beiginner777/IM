<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import { Table, Tag, Button, Empty } from 'ant-design-vue'
import { ArrowLeftOutlined } from '@ant-design/icons-vue'
import { useProductStore } from '../store/product'
import { useAuthStore } from '../store/auth'

const router = useRouter()
const productStore = useProductStore()
const authStore = useAuthStore()
const loading = ref(false)

const columns = [
  {
    title: '订单编号',
    dataIndex: 'orderId',
    key: 'orderId',
    width: 120,
  },
  {
    title: '商品名称',
    dataIndex: 'productName',
    key: 'productName',
  },
  {
    title: '购买时间',
    dataIndex: 'time',
    key: 'time',
    width: 200,
  },
  {
    title: '状态',
    dataIndex: 'status',
    key: 'status',
    width: 100,
  },
]

onMounted(async () => {
  if (!authStore.serverInfo?.username) return
  loading.value = true
  await productStore.fetchOrders(authStore.serverInfo.username)
  loading.value = false
})

function showTotal(total: number) {
  return `共 ${total} 条记录`
}
</script>

<template>
  <div class="orders-page">
    <header class="page-header">
      <div class="header-content">
        <div class="header-left">
          <Button type="text" @click="router.push('/products')">
            <template #icon><ArrowLeftOutlined /></template>
            返回商城
          </Button>
          <h1 class="header-title">我的抢购记录</h1>
        </div>
      </div>
    </header>

    <main class="page-main">
      <div v-if="!loading && productStore.orders.length === 0" class="empty-state">
        <Empty description="暂无抢购记录">
          <Button type="primary" @click="router.push('/products')">
            去抢购
          </Button>
        </Empty>
      </div>

      <div v-else class="table-wrapper">
        <Table
          :columns="columns"
          :dataSource="productStore.orders"
          :loading="loading"
          :pagination="{ pageSize: 10, showTotal }"
          rowKey="orderId"
          bordered
        >
          <template #bodyCell="{ column, text }">
            <template v-if="column.key === 'status'">
              <Tag :color="text === '成功' ? 'green' : 'red'">{{ text }}</Tag>
            </template>
          </template>
        </Table>
      </div>
    </main>
  </div>
</template>

<style scoped>
.orders-page {
  min-height: 100vh;
  background: #f0f2f5;
}

.page-header {
  background: #fff;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.header-content {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 24px;
  height: 64px;
  display: flex;
  align-items: center;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 16px;
}

.header-title {
  font-size: 20px;
  font-weight: 700;
  color: #1a1a2e;
  margin: 0;
}

.page-main {
  max-width: 1200px;
  margin: 0 auto;
  padding: 24px;
}

.table-wrapper {
  background: #fff;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.empty-state {
  padding: 80px 0;
  text-align: center;
  background: #fff;
  border-radius: 12px;
}
</style>
