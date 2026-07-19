<script setup lang="ts">
import { useRouter } from 'vue-router'
import { Button } from 'ant-design-vue'
import { useAuthStore } from '../store/auth'

const router = useRouter()
const authStore = useAuthStore()
</script>

<template>
  <div class="home-page">
    <!-- 光晕 -->
    <div class="glow glow-1" />
    <div class="glow glow-2" />

    <!-- 背景粒子 -->
    <div class="particles">
      <span v-for="i in 25" :key="i" class="particle" :style="{
        width: (Math.random() * 5 + 2) + 'px',
        height: (Math.random() * 5 + 2) + 'px',
        left: Math.random() * 100 + '%',
        bottom: -(Math.random() * 40 + 10) + 'px',
        animationDuration: (Math.random() * 12 + 8) + 's',
        animationDelay: Math.random() * 10 + 's',
      }" />
    </div>

    <!-- 主体 -->
    <section class="hero">
      <div class="hero-badge">🚀 高性能秒杀引擎</div>
      <h1>限时<span class="gradient"> 秒杀 </span>· 手速决胜</h1>
      <p class="hero-desc">
        基于 C++ Beast 构建的高并发秒杀平台<br />
        Redis Lua 原子脚本防超卖 · RabbitMQ 异步削峰 · 毫秒级抢购体验
      </p>
      <div class="hero-actions">
        <template v-if="authStore.isLoggedIn">
          <Button type="primary" size="large" class="btn-fill" @click="router.push('/products')">
            进入商城 →
          </Button>
        </template>
        <template v-else>
          <Button type="primary" size="large" class="btn-fill" @click="router.push('/login')">
            立即抢购 →
          </Button>
          <Button size="large" class="btn-outline" @click="router.push('/register')" ghost>
            创建账号
          </Button>
        </template>
      </div>
    </section>

    <!-- 特性卡片 -->
    <section class="features">
      <div class="feature-card">
        <div class="feature-icon">⚛️</div>
        <div class="feature-title">Redis Lua 原子扣减</div>
        <div class="feature-desc">单线程 Lua 脚本保证库存扣减原子性，杜绝超卖</div>
      </div>
      <div class="feature-card">
        <div class="feature-icon">📨</div>
        <div class="feature-title">消息队列异步削峰</div>
        <div class="feature-desc">RabbitMQ 将订单写入、通知、排行榜异步并行处理</div>
      </div>
      <div class="feature-card">
        <div class="feature-icon">🖥️</div>
        <div class="feature-title">C++ Beast 高性能</div>
        <div class="feature-desc">Boost Beast HTTP 服务，微秒级请求处理延迟</div>
      </div>
      <div class="feature-card">
        <div class="feature-icon">📊</div>
        <div class="feature-title">实时排行榜</div>
        <div class="feature-desc">Redis ZSET 毫秒级更新抢购排名，数据实时可见</div>
      </div>
    </section>

    <footer class="home-footer">
      Built with C++ · Boost Beast · Redis · RabbitMQ &nbsp;|&nbsp; IM Flash Sale Platform
    </footer>
  </div>
</template>

<style scoped>
.home-page {
  min-height: calc(100vh - 56px);
  background: linear-gradient(160deg, #0f0c29 0%, #1a1a3e 30%, #24243e 60%, #1a1a3e 100%);
  display: flex; flex-direction: column; align-items: center;
  padding: 80px 24px 60px;
  position: relative; overflow: hidden;
}

.glow {
  position: fixed; border-radius: 50%; filter: blur(140px); opacity: 0.10; pointer-events: none;
}
.glow-1 { width: 700px; height: 700px; background: #667eea; top: -250px; right: -200px; }
.glow-2 { width: 500px; height: 500px; background: #f093fb; bottom: -200px; left: -150px; }

.particles { position: fixed; inset: 0; pointer-events: none; }
.particle {
  position: absolute; border-radius: 50%; background: rgba(102,126,234,0.25);
  animation: drift linear infinite;
}
@keyframes drift {
  0%   { transform: translateY(0) translateX(0) scale(1); opacity: 0; }
  10%  { opacity: 0.6; }
  90%  { opacity: 0.6; }
  100% { transform: translateY(-100vh) translateX(40px) scale(0.5); opacity: 0; }
}

/* hero */
.hero {
  text-align: center; max-width: 780px; position: relative; z-index: 1;
  animation: fadeUp 0.8s ease-out;
}
@keyframes fadeUp { from { opacity: 0; transform: translateY(30px) } to { opacity: 1; transform: translateY(0) } }

.hero-badge {
  display: inline-block; padding: 6px 18px; border-radius: 20px;
  border: 1px solid rgba(102,126,234,0.35); font-size: 13px;
  color: #667eea; letter-spacing: 2px; margin-bottom: 28px;
}
.hero h1 {
  font-size: 54px; font-weight: 800; color: #fff; line-height: 1.3; margin-bottom: 20px;
}
.hero h1 .gradient {
  background: linear-gradient(135deg, #667eea 20%, #f093fb 80%);
  -webkit-background-clip: text; -webkit-text-fill-color: transparent;
}
.hero-desc {
  font-size: 17px; color: rgba(255,255,255,0.5); line-height: 1.8; margin-bottom: 40px;
}
.hero-actions { display: flex; gap: 16px; justify-content: center; }

.btn-fill {
  background: linear-gradient(135deg, #667eea, #764ba2) !important;
  border: none !important; color: #fff !important; font-weight: 700;
  box-shadow: 0 4px 20px rgba(102,126,234,0.4) !important;
}
.btn-outline { border-color: rgba(255,255,255,0.2) !important; color: rgba(255,255,255,0.8) !important; }

/* features */
.features {
  display: grid; grid-template-columns: repeat(4, 1fr); gap: 24px;
  max-width: 960px; width: 100%; margin-top: 72px; position: relative; z-index: 1;
}
.feature-card {
  background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.06);
  border-radius: 18px; padding: 32px 24px; text-align: center;
  transition: all 0.3s ease;
}
.feature-card:hover {
  background: rgba(255,255,255,0.06); border-color: rgba(102,126,234,0.25);
  transform: translateY(-4px);
}
.feature-icon { font-size: 36px; margin-bottom: 16px; }
.feature-title { font-size: 15px; font-weight: 700; margin-bottom: 8px; color: #e0e0e0; }
.feature-desc  { font-size: 13px; color: rgba(255,255,255,0.4); line-height: 1.6; }

.home-footer {
  text-align: center; padding: 32px; margin-top: auto;
  font-size: 13px; color: rgba(255,255,255,0.18); letter-spacing: 1px; position: relative; z-index: 1;
}

@media (max-width: 768px) {
  .hero h1 { font-size: 32px; }
  .features { grid-template-columns: repeat(2, 1fr); gap: 12px; }
}
</style>
