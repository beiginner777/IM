import request from './request'

// ==================== 类型定义 ====================

export interface LoginParams {
  username: string
  password: string
}

export interface LoginResult {
  token: string
  uid: string
}

export interface RegisterParams {
  username: string
  password: string
}

export interface RegisterResult {
  token: string
  uid: string
}

export interface Product {
  id: number
  name: string
  price: number
  stock: number
  imageUrl: string
}

export interface BuyResult {
  success: boolean
  message: string
}

export interface RankItem {
  productId: number
  productName: string
  count: number
}

export interface Order {
  orderId: number
  productName: string
  time: string
  status: '成功' | '失败' | string
}

// ==================== API 函数 ====================

/** 登录 */
export function login(params: LoginParams) {
  return request.post<LoginResult>('/login', params)
}

/** 注册 */
export function register(params: RegisterParams) {
  return request.post<RegisterResult>('/register', params)
}

/** 获取商品列表 */
export function getProducts() {
  return request.get<Product[]>('/products')
}

/** 秒杀购买 */
export function buyProduct(productId: number) {
  return request.post<BuyResult>(`/buy/${productId}`)
}

/** 获取排行榜 */
export function getRank() {
  return request.get<RankItem[]>('/rank')
}

/** 获取订单记录 */
export function getOrders(uid: string) {
  return request.get<Order[]>('/orders', { params: { uid } })
}
