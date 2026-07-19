import axios from 'axios'
import { message } from 'antd'
import { getServerInfo } from '../utils/token'

const request = axios.create({ baseURL: '/api', timeout: 10000, headers: { 'Content-Type': 'application/json' } })

export function setBaseURL(host: string, port: number) {
  request.defaults.baseURL = `http://${host}:${port}`
}
export function resetBaseURL() {
  request.defaults.baseURL = '/api'
}

request.interceptors.response.use(
  (res) => {
    if (res.data && typeof res.data.error_code === 'number' && res.data.error_code !== 0) {
      message.error(res.data.error_msg || '请求失败')
      return Promise.reject(new Error(res.data.error_msg))
    }
    return res
  },
  (err) => {
    if (err.response) {
      const { status, data } = err.response
      if (data?.error_msg) { message.error(data.error_msg) }
      else if (status === 401) { message.error('登录已过期'); window.location.href = '/login' }
      else { message.error(data?.message || `请求失败 (${status})`) }
    } else if (err.code === 'ECONNABORTED') { message.error('请求超时') }
    else { message.error('网络错误') }
    return Promise.reject(err)
  }
)

// refresh 后从 localStorage 恢复 baseURL
const saved = getServerInfo()
if (saved) request.defaults.baseURL = `http://${saved.host}:${saved.port}`

export default request
