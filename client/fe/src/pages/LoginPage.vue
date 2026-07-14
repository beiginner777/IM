<script setup lang="ts">
import { ref, reactive } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { Form, Input, Button, Card, message } from 'ant-design-vue'
import { UserOutlined, LockOutlined } from '@ant-design/icons-vue'
import { login } from '../api'
import { useAuthStore } from '../store/auth'

const router = useRouter()
const route = useRoute()
const authStore = useAuthStore()

const formRef = ref()
const loading = ref(false)
const formState = reactive({
  username: '',
  password: '',
})

const rules: Record<string, any> = {
  username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
  password: [{ required: true, message: '请输入密码', trigger: 'blur' }],
}

async function handleSubmit() {
  try {
    await formRef.value.validate()
    loading.value = true
    const res = await login({
      username: formState.username,
      password: formState.password,
    })
    const { token, uid } = res.data
    authStore.loginSuccess(token, uid, formState.username)
    message.success('登录成功')
    const redirect = (route.query.redirect as string) || '/products'
    router.push(redirect)
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
  <div class="login-container">
    <Card class="login-card" :bordered="false">
      <h1 class="login-title">秒杀商城</h1>
      <p class="login-subtitle">IM 账号登录</p>

      <Form
        ref="formRef"
        :model="formState"
        :rules="rules"
        layout="vertical"
        @keyup.enter="handleSubmit"
      >
        <Form.Item name="username">
          <Input
            v-model:value="formState.username"
            size="large"
            placeholder="用户名"
            :prefix="UserOutlined"
          />
        </Form.Item>

        <Form.Item name="password">
          <Input.Password
            v-model:value="formState.password"
            size="large"
            placeholder="密码"
            :prefix="LockOutlined"
          />
        </Form.Item>

        <Form.Item>
          <Button
            type="primary"
            size="large"
            block
            :loading="loading"
            @click="handleSubmit"
          >
            登录
          </Button>
        </Form.Item>
      </Form>

      <div class="login-footer">
        还没有账号？
        <router-link to="/register">立即注册</router-link>
      </div>
    </Card>
  </div>
</template>

<style scoped>
.login-container {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.login-card {
  width: 400px;
  border-radius: 12px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.15);
}

.login-title {
  text-align: center;
  font-size: 28px;
  font-weight: 700;
  color: #1a1a2e;
  margin-bottom: 4px;
}

.login-subtitle {
  text-align: center;
  color: #8c8c8c;
  margin-bottom: 32px;
  font-size: 14px;
}

.login-footer {
  text-align: center;
  color: #8c8c8c;
  font-size: 14px;
}

.login-footer a {
  color: #667eea;
  font-weight: 500;
}
</style>
