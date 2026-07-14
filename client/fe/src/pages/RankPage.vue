<script setup lang="ts">
import { useRouter } from 'vue-router'
import { Button } from 'ant-design-vue'
import { ArrowLeftOutlined } from '@ant-design/icons-vue'
import RankList from '../components/RankList.vue'

const router = useRouter()
</script>

<template>
  <div class="rank-page">
    <header class="page-header">
      <div class="header-content">
        <div class="header-left">
          <Button type="text" @click="router.push('/products')">
            <template #icon><ArrowLeftOutlined /></template>
            返回商城
          </Button>
          <h1 class="header-title">实时排行榜</h1>
        </div>
      </div>
    </header>

    <main class="page-main">
      <div class="rank-wrapper">
        <RankList />
      </div>
    </main>
  </div>
</template>

<style scoped>
.rank-page {
  min-height: 100vh;
  background: #f0f2f5;
}

.page-header {
  background: #fff;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.header-content {
  max-width: 900px;
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
  max-width: 900px;
  margin: 0 auto;
  padding: 24px;
}

.rank-wrapper {
  /* RankList 自带背景、圆角和阴影 */
}
</style>
