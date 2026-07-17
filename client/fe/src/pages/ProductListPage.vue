<script setup lang="ts">
import { onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'
import { Button, Spin, Empty } from 'ant-design-vue'
import { LogoutOutlined, OrderedListOutlined, TrophyOutlined } from '@ant-design/icons-vue'
import { useProductStore } from '../store/product'
import { useAuthStore } from '../store/auth'
import ProductCard from '../components/ProductCard.vue'
import RankList from '../components/RankList.vue'

const router = useRouter()
const productStore = useProductStore()
const authStore = useAuthStore()

onMounted(() => {
  productStore.fetchProducts()
})

function handleLogout() {
  authStore.logout()
  router.push('/login')
}
</script>

<template>
  <div class="products-page">
    <!-- 顶部导航栏 -->
    <header class="page-header">
      <div class="header-content">
        <h1 class="header-logo">⚡ 秒杀商城</h1>
        <div class="header-actions">
          <span class="welcome-text">欢迎，{{ authStore.username }}</span>
          <Button type="text" @click="router.push('/rank')">
            <template #icon><TrophyOutlined /></template>
            排行榜
          </Button>
          <Button type="text" @click="router.push('/orders')">
            <template #icon><OrderedListOutlined /></template>
            我的记录
          </Button>
          <Button type="text" danger @click="handleLogout">
            <template #icon><LogoutOutlined /></template>
            退出
          </Button>
        </div>
      </div>
    </header>

    <!-- 主内容区 -->
    <main class="page-main">
      <div class="main-content">
        <div class="products-section">
          <h2 class="section-title">🔥 限时秒杀</h2>

          <Spin :spinning="productStore.loading" tip="加载中...">
            <div v-if="!productStore.loading && productStore.products.length === 0" class="empty-state">
              <Empty description="暂无商品" />
            </div>

            <div v-else class="products-grid">
              <ProductCard
                v-for="product in productStore.products"
                :key="product.id"
                :product="product"
              />
            </div>
          </Spin>
        </div>

        <!-- 侧边栏排行榜 -->
        <aside class="rank-sidebar">
          <RankList />
        </aside>
      </div>
    </main>
  </div>
</template>

<style scoped>
.products-page {
  min-height: 100vh;
  background: #f0f2f5;
}

.page-header {
  background: #fff;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
  position: sticky;
  top: 0;
  z-index: 100;
}

.header-content {
  max-width: 1400px;
  margin: 0 auto;
  padding: 0 24px;
  height: 64px;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.header-logo {
  font-size: 22px;
  font-weight: 800;
  color: #1a1a2e;
  margin: 0;
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.welcome-text {
  color: #595959;
  font-size: 14px;
  margin-right: 8px;
}

.page-main {
  max-width: 1400px;
  margin: 0 auto;
  padding: 24px;
}

.main-content {
  display: flex;
  gap: 24px;
  align-items: flex-start;
}

.products-section {
  flex: 1;
  min-width: 0;
}

.section-title {
  font-size: 22px;
  font-weight: 700;
  color: #1a1a2e;
  margin-bottom: 20px;
}

.products-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 20px;
}

.rank-sidebar {
  width: 380px;
  flex-shrink: 0;
  position: sticky;
  top: 88px;
}

.empty-state {
  padding: 80px 0;
  text-align: center;
}

@media (max-width: 1024px) {
  .main-content {
    flex-direction: column;
  }

  .rank-sidebar {
    width: 100%;
    position: static;
  }
}

@media (max-width: 640px) {
  .products-grid {
    grid-template-columns: 1fr;
  }

  .header-actions {
    gap: 4px;
  }

  .welcome-text {
    display: none;
  }
}
</style>
