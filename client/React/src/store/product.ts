import { create } from 'zustand'
import type { Product, RankItem, Order } from '../api'
import { getProducts, getRank, getOrders } from '../api'

interface ProductState {
  products: Product[]; rankList: RankItem[]; orders: Order[]; loading: boolean
  fetchProducts: () => Promise<void>
  fetchRank: () => Promise<void>
  fetchOrders: (u: string) => Promise<void>
  decrementStock: (id: number) => void
}

export const useProductStore = create<ProductState>((set, get) => ({
  products: [], rankList: [], orders: [], loading: false,
  fetchProducts: async () => {
    set({ loading: true })
    try { const res = await getProducts(); set({ products: res.data }) }
    finally { set({ loading: false }) }
  },
  fetchRank: async () => {
    try { const res = await getRank(); set({ rankList: res.data }) } catch {}
  },
  fetchOrders: async (u) => {
    try { const res = await getOrders(u); set({ orders: res.data }) } catch {}
  },
  decrementStock: (id) => set((s) => ({
    products: s.products.map(p => p.id === id && p.stock > 0 ? { ...p, stock: p.stock - 1 } : p)
  })),
}))
