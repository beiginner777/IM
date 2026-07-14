<script setup lang="ts">
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { Form, Input, Button, Card, message } from 'ant-design-vue'
import { UserOutlined, LockOutlined } from '@ant-design/icons-vue'
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
  <div class="register-container">
    <Card class="register-card" :bordered="false">
      <h1 class="register-title">创建账号</h1>
      <p class="register-subtitle">注册一个 IM 账号开始秒杀</p>

      <Form
        ref="formRef"
        :model="formState"
        :rules="rules"
        layout="vertical"
      >
        <Form.Item name="username">
          <Input
            v-model:value="formState.username"
            size="large"
            placeholder="用户名（3-20 个字符）"
            :prefix="UserOutlined"
          />
        </Form.Item>

        <Form.Item name="password">
          <Input.Password
            v-model:value="formState.password"
            size="large"
            placeholder="密码（至少 6 个字符）"
            :prefix="LockOutlined"
          />
        </Form.Item>

        <Form.Item name="confirmPassword">
          <Input.Password
            v-model:value="formState.confirmPassword"
            size="large"
            placeholder="确认密码"
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
            注册
          </Button>
        </Form.Item>
      </Form>

      <div class="register-footer">
        已有账号？
        <router-link to="/login">立即登录</router-link>
      </div>
    </Card>
  </div>
</template>

<style scoped>
.register-container {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.register-card {
  width: 420px;
  border-radius: 12px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.15);
}

.register-title {
  text-align: center;
  font-size: 28px;
  font-weight: 700;
  color: #1a1a2e;
  margin-bottom: 4px;
}

.register-subtitle {
  text-align: center;
  color: #8c8c8c;
  margin-bottom: 32px;
  font-size: 14px;
}

.register-footer {
  text-align: center;
  color: #8c8c8c;
  font-size: 14px;
}

.register-footer a {
  color: #667eea;
  font-weight: 500;
}
</style>
