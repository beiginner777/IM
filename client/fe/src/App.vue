<script setup lang="ts">
import { useRouter } from 'vue-router'
import { useAuthStore } from './store/auth'
import { Button } from 'ant-design-vue'
import { ThunderboltOutlined, UserOutlined } from '@ant-design/icons-vue'

const router = useRouter()
const authStore = useAuthStore()

function handleLogout() {
  authStore.logout()
  router.push('/login')
}
</script>

<template>
  <div id="app-shell">
    <!-- 全局导航栏 -->
    <header class="app-navbar">
      <div class="nav-left">
        <router-link to="/" class="nav-brand">
          <span class="nav-icon"><ThunderboltOutlined /></span>
          秒杀商城
        </router-link>
      </div>
      <div class="nav-right">
        <template v-if="authStore.isLoggedIn">
          <span class="nav-user"><UserOutlined /> {{ authStore.username }}</span>
          <router-link to="/products">
            <Button type="text" style="color: rgba(255,255,255,0.7)">商品</Button>
          </router-link>
          <router-link to="/orders">
            <Button type="text" style="color: rgba(255,255,255,0.7)">订单</Button>
          </router-link>
          <router-link to="/rank">
            <Button type="text" style="color: rgba(255,255,255,0.7)">排行</Button>
          </router-link>
          <Button type="text" danger @click="handleLogout">退出</Button>
        </template>
        <template v-else>
          <router-link to="/login">
            <Button type="primary" ghost size="small">登 录</Button>
          </router-link>
          <router-link to="/register">
            <Button type="text" size="small" style="color: rgba(255,255,255,0.7)">注 册</Button>
          </router-link>
        </template>
      </div>
    </header>

    <!-- 页面内容 -->
    <main class="app-main">
      <router-view />
    </main>
  </div>
</template>

<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, 'PingFang SC', 'Microsoft YaHei', sans-serif;
}

#app-shell {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.app-navbar {
  position: sticky; top: 0; z-index: 1000;
  display: flex; align-items: center; justify-content: space-between;
  padding: 0 32px; height: 56px;
  background: rgba(15, 12, 41, 0.85);
  backdrop-filter: blur(16px);
  border-bottom: 1px solid rgba(255,255,255,0.06);
}
.nav-left, .nav-right { display: flex; align-items: center; gap: 16px; }
.nav-brand {
  display: flex; align-items: center; gap: 10px;
  font-size: 18px; font-weight: 800; color: #fff; text-decoration: none; letter-spacing: 2px;
}
.nav-icon {
  display: inline-flex; align-items: center; justify-content: center;
  width: 32px; height: 32px; border-radius: 8px;
  background: linear-gradient(135deg, #667eea, #764ba2);
  font-size: 16px; color: #fff;
}
.nav-user { color: rgba(255,255,255,0.6); font-size: 14px; }

.app-main { flex: 1; }
</style>
