import request, { setBaseURL } from './request'

export interface LoginParams { username: string; password: string }
export interface LoginResult { username: string; host: string; port: number }
export interface RegisterParams { username: string; password: string }
export interface RegisterResult { username: string; host: string; port: number }
export interface Product { id: number; name: string; price: number; stock: number; imageUrl: string }
export interface BuyResult { success: boolean; message: string }
export interface RankItem { productId: number; productName: string; count: number }
export interface Order { orderId: number; productName: string; time: string; status: string }

export const login    = (p: LoginParams)    => request.post<LoginResult>('/login', p)
export const register = (p: RegisterParams) => request.post<RegisterResult>('/register', p)
export const getProducts = ()       => request.get<Product[]>('/products')
export const buyProduct  = (id: number, password: string) => request.post<BuyResult>(`/buy/${id}`, { password })
export const getRank     = ()       => request.get<RankItem[]>('/rank')
export const getOrders   = (u: string) => request.get<Order[]>('/orders', { params: { username: u } })

export { setBaseURL }
