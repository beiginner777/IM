<template>
  <div class="recharge-container">
    <div class="recharge-card">
      <h1>账户充值</h1>
      <div class="balance-row">
        <span>当前余额：</span>
        <span class="balance-amount">¥ {{ authStore.balance.toFixed(2) }}</span>
      </div>
      <a-form :model="form" layout="vertical" @finish="handleRecharge">
        <a-form-item label="充值金额">
          <a-input-number v-model:value="form.amount" :min="1" :precision="2" style="width:100%" placeholder="请输入金额" />
        </a-form-item>
        <a-form-item label="登录密码">
          <a-input-password v-model:value="form.password" placeholder="请输入密码确认" />
        </a-form-item>
        <a-form-item>
          <a-button type="primary" html-type="submit" :loading="loading" block>确认充值</a-button>
        </a-form-item>
      </a-form>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { message } from 'ant-design-vue'
import { useAuthStore } from '../store/auth'
import request from '../api/request'

const authStore = useAuthStore()
const loading = ref(false)
const form = reactive({ amount: 100, password: '' })

async function handleRecharge() {
  if (!form.password) { message.error('请输入密码'); return }
  loading.value = true
  try {
    const res = await request.post('/recharge', { amount: form.amount, password: form.password })
    if (res.data.code === 0) {
      authStore.setBalance(res.data.balance)
      message.success(`充值成功！余额 ¥${res.data.balance.toFixed(2)}`)
      form.password = ''
    } else {
      message.error(res.data.message || '充值失败')
    }
  } catch { message.error('网络错误') }
  finally { loading.value = false }
}
</script>

<style scoped>
.recharge-container { display:flex; justify-content:center; align-items:center; min-height:100vh; background:#0a0a1a; }
.recharge-card { background:rgba(255,255,255,0.04); border:1px solid rgba(255,255,255,0.08); border-radius:16px; padding:40px; width:420px; }
h1 { color:#fff; text-align:center; margin-bottom:24px; }
.balance-row { text-align:center; color:#aaa; margin-bottom:24px; }
.balance-amount { color:#ffd700; font-size:28px; font-weight:bold; }
</style>
