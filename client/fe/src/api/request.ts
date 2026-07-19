import axios from 'axios'
import { message } from 'ant-design-vue'
import { getServerInfo, getToken } from '../utils/token'

// ==================== 请求拦截器：附带 JWT ====================
request.interceptors.request.use((config) => {
  const token = getToken()
  if (token) config.headers.Authorization = `Bearer ${token}`
  return config
})

const request = axios.create({
  baseURL: '/api',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
})

// ==================== 动态切换服务器地址 ====================

/** 登录成功后调用，将后续请求切到 GateServer 返回的地址 */
export function setBaseURL(host: string, port: number) {
  request.defaults.baseURL = `http://${host}:${port}`
}

export function getBaseURL(): string {
  return request.defaults.baseURL as string
}

// ==================== 响应拦截器 ====================

request.interceptors.response.use(
  (response) => {
    const data = response.data
    // 如果响应体中包含非零 error_code，统一当作业务错误处理
    if (data && typeof data.error_code === 'number' && data.error_code !== 0) {
      message.error(data.error_msg || '请求失败')
      return Promise.reject(new Error(data.error_msg || '请求失败'))
    }
    return response
  },
  (error) => {
    if (error.response) {
      const { status, data } = error.response

      // 优先使用服务端返回的 error_msg
      if (data?.error_msg) {
        message.error(data.error_msg)
      } else {
        switch (status) {
          case 401:
            message.error('登录已过期，请重新登录')
            window.location.href = '/login'
            break
          case 400:
            message.error(data?.message || '请求参数错误')
            break
          case 500:
            message.error(data?.message || '服务器内部错误')
            break
          default:
            message.error(data?.message || `请求失败 (${status})`)
        }
      }
    } else if (error.code === 'ECONNABORTED') {
      message.error('请求超时，请稍后重试')
    } else {
      message.error('网络错误，请检查网络连接')
    }
    return Promise.reject(error)
  },
)

// 页面刷新后从 localStorage 恢复 baseURL ——
// 否则刷新后 axios 回到默认 /api，所有业务请求打到 GateServer 拿 HTML 而非 JSON
const saved = getServerInfo()
if (saved) {
  request.defaults.baseURL = `http://${saved.host}:${saved.port}`
}

export default request
