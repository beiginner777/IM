<script setup lang="ts">
import { ref, reactive, h } from 'vue'
import { useRouter } from 'vue-router'
import { Form, Input, Button, message } from 'ant-design-vue'
import { UserOutlined, LockOutlined, UserAddOutlined } from '@ant-design/icons-vue'
import { register } from '../api'
import { useAuthStore } from '../store/auth'

const router = useRouter()
const authStore = useAuthStore()

const formRef = ref()
const loading = ref(false)
const formState = reactive({
  username: '',
  password: '',
  confirmPassword: '',
})

const validateConfirmPassword = (_rule: any, value: string) => {
  if (!value) {
    return Promise.reject('请再次输入密码')
  }
  if (value !== formState.password) {
    return Promise.reject('两次输入的密码不一致')
  }
  return Promise.resolve()
}

const rules: Record<string, any> = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '用户名长度 3-20 个字符', trigger: 'blur' },
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, message: '密码至少 6 个字符', trigger: 'blur' },
  ],
  confirmPassword: [{ validator: validateConfirmPassword, trigger: 'blur' }],
}

async function handleSubmit() {
  try {
    await formRef.value.validate()
    loading.value = true
    const res = await register({
      username: formState.username,
      password: formState.password,
    })
    const { token, uid } = res.data
    authStore.loginSuccess(token, uid, formState.username)
    message.success('注册成功')
    router.push('/products')
  } catch (err: any) {
    if (err?.response?.data?.message) {
      message.error(err.response.data.message)
    }
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <div class="auth-page">
    <!-- 背景装饰 -->
    <div class="bg-decoration">
      <div class="bg-circle bg-circle-1" />
      <div class="bg-circle bg-circle-2" />
      <div class="bg-circle bg-circle-3" />
      <div class="bg-grid" />
    </div>

    <!-- 卡片 -->
    <div class="auth-card">
      <!-- 品牌区 -->
      <div class="brand">
        <div class="brand-icon">
          <UserAddOutlined />
        </div>
        <h1 class="brand-name">创建账号</h1>
        <p class="brand-desc">注册 IM 账号，开始秒杀之旅</p>
      </div>

      <!-- 表单 -->
      <Form
        ref="formRef"
        :model="formState"
        :rules="rules"
        class="auth-form"
      >
        <Form.Item name="username">
          <Input
            v-model:value="formState.username"
            size="large"
            placeholder="请输入用户名（3-20 个字符）"
            class="auth-input"
            :prefix="h(UserOutlined)"
          />
        </Form.Item>

        <Form.Item name="password">
          <Input.Password
            v-model:value="formState.password"
            size="large"
            placeholder="请输入密码（至少 6 个字符）"
            class="auth-input"
            :prefix="h(LockOutlined)"
          />
        </Form.Item>

        <Form.Item name="confirmPassword">
          <Input.Password
            v-model:value="formState.confirmPassword"
            size="large"
            placeholder="请再次输入密码"
            class="auth-input"
            :prefix="h(LockOutlined)"
          />
        </Form.Item>

        <Form.Item>
          <Button
            type="primary"
            size="large"
            block
            :loading="loading"
            class="auth-btn"
            @click="handleSubmit"
          >
            注 册
          </Button>
        </Form.Item>
      </Form>

      <!-- 底部切换 -->
      <div class="auth-switch">
        <span>已有账号？</span>
        <router-link to="/login">立即登录 →</router-link>
      </div>
    </div>
  </div>
</template>

<style scoped>
/* ========== 页面容器 ========== */
.auth-page {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(160deg, #0f0c29 0%, #1a1a3e 30%, #24243e 60%, #1a1a3e 100%);
  position: relative;
  overflow: hidden;
  font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, sans-serif;
}

/* ========== 背景装饰层 ========== */
.bg-decoration {
  position: absolute;
  inset: 0;
  pointer-events: none;
}

.bg-circle {
  position: absolute;
  border-radius: 50%;
  filter: blur(80px);
  opacity: 0.15;
}

.bg-circle-1 {
  width: 520px;
  height: 520px;
  background: #4facfe;
  top: -160px;
  right: -120px;
  animation: float1 20s ease-in-out infinite;
}

.bg-circle-2 {
  width: 400px;
  height: 400px;
  background: #f093fb;
  bottom: -140px;
  left: -60px;
  animation: float2 25s ease-in-out infinite;
}

.bg-circle-3 {
  width: 300px;
  height: 300px;
  background: #43e97b;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  animation: float3 18s ease-in-out infinite;
}

@keyframes float1 {
  0%, 100% { transform: translate(0, 0) scale(1); }
  33%  { transform: translate(-40px, 30px) scale(1.08); }
  66%  { transform: translate(20px, -20px) scale(0.94); }
}

@keyframes float2 {
  0%, 100% { transform: translate(0, 0) scale(1); }
  50%  { transform: translate(30px, -40px) scale(1.1); }
}

@keyframes float3 {
  0%, 100% { transform: translate(-50%, -50%) scale(1); }
  50%  { transform: translate(-50%, -50%) scale(1.15); }
}

.bg-grid {
  position: absolute;
  inset: 0;
  background-image:
    linear-gradient(rgba(255,255,255,0.02) 1px, transparent 1px),
    linear-gradient(90deg, rgba(255,255,255,0.02) 1px, transparent 1px);
  background-size: 60px 60px;
  mask-image: radial-gradient(ellipse at center, black 30%, transparent 70%);
}

/* ========== 卡片 ========== */
.auth-card {
  position: relative;
  z-index: 1;
  width: 420px;
  padding: 48px 40px 40px;
  background: rgba(255, 255, 255, 0.04);
  backdrop-filter: blur(24px);
  -webkit-backdrop-filter: blur(24px);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 24px;
  box-shadow:
    0 4px 24px rgba(0, 0, 0, 0.3),
    0 1px 2px rgba(255, 255, 255, 0.06) inset;
  animation: cardFadeIn 0.6s ease-out;
}

@keyframes cardFadeIn {
  from {
    opacity: 0;
    transform: translateY(20px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

/* ========== 品牌区 ========== */
.brand {
  text-align: center;
  margin-bottom: 36px;
}

.brand-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 56px;
  height: 56px;
  border-radius: 16px;
  background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
  font-size: 28px;
  color: #fff;
  margin-bottom: 16px;
  box-shadow: 0 8px 24px rgba(79, 172, 254, 0.35);
}

.brand-name {
  font-size: 28px;
  font-weight: 800;
  color: #fff;
  margin: 0 0 6px;
  letter-spacing: 2px;
}

.brand-desc {
  font-size: 14px;
  color: rgba(255, 255, 255, 0.45);
  margin: 0;
}

/* ========== 表单 ========== */
.auth-form :deep(.ant-form-item) {
  margin-bottom: 20px;
}

.auth-input :deep(.ant-input),
.auth-input :deep(.ant-input-affix-wrapper) {
  background: rgba(255, 255, 255, 0.06) !important;
  border: 1px solid rgba(255, 255, 255, 0.1) !important;
  border-radius: 12px !important;
  color: #fff !important;
  transition: all 0.3s ease;
}

.auth-input :deep(.ant-input::placeholder) {
  color: rgba(255, 255, 255, 0.3);
}

.auth-input :deep(.ant-input-affix-wrapper) {
  padding: 8px 14px !important;
}

.auth-input :deep(.ant-input-affix-wrapper:hover),
.auth-input :deep(.ant-input:hover) {
  border-color: rgba(255, 255, 255, 0.25) !important;
  background: rgba(255, 255, 255, 0.08) !important;
}

.auth-input :deep(.ant-input-affix-wrapper-focused),
.auth-input :deep(.ant-input-affix-wrapper:focus-within) {
  border-color: #4facfe !important;
  box-shadow: 0 0 0 3px rgba(79, 172, 254, 0.15) !important;
  background: rgba(255, 255, 255, 0.08) !important;
}

.auth-input :deep(.ant-input-password-icon) {
  color: rgba(255, 255, 255, 0.35) !important;
}

.auth-input :deep(.ant-input-password-icon:hover) {
  color: rgba(255, 255, 255, 0.65) !important;
}

.input-icon {
  color: rgba(255, 255, 255, 0.35);
  font-size: 16px;
}

/* ========== 按钮 ========== */
.auth-btn {
  height: 48px !important;
  border-radius: 12px !important;
  font-size: 16px !important;
  font-weight: 700 !important;
  letter-spacing: 4px;
  border: none !important;
  background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%) !important;
  box-shadow: 0 4px 16px rgba(79, 172, 254, 0.4);
  transition: all 0.3s ease !important;
  margin-top: 8px;
}

.auth-btn:hover {
  transform: translateY(-1px);
  box-shadow: 0 6px 24px rgba(79, 172, 254, 0.55) !important;
}

.auth-btn:active {
  transform: translateY(0);
}

/* ========== 底部切换 ========== */
.auth-switch {
  text-align: center;
  margin-top: 28px;
  font-size: 14px;
  color: rgba(255, 255, 255, 0.4);
}

.auth-switch a {
  color: #4facfe;
  font-weight: 600;
  margin-left: 4px;
  transition: color 0.2s;
}

.auth-switch a:hover {
  color: #7ec8fe;
}
</style>
