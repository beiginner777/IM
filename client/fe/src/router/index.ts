import { createRouter, createWebHistory } from 'vue-router'
import { isLoggedIn } from '../utils/token'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/login',
      name: 'Login',
      component: () => import('../pages/LoginPage.vue'),
      meta: { guest: true },
    },
    {
      path: '/register',
      name: 'Register',
      component: () => import('../pages/RegisterPage.vue'),
      meta: { guest: true },
    },
    {
      path: '/',
      redirect: '/products',
    },
    {
      path: '/products',
      name: 'Products',
      component: () => import('../pages/ProductListPage.vue'),
      meta: { requiresAuth: true },
    },
    {
      path: '/orders',
      name: 'Orders',
      component: () => import('../pages/OrdersPage.vue'),
      meta: { requiresAuth: true },
    },
    {
      path: '/rank',
      name: 'Rank',
      component: () => import('../pages/RankPage.vue'),
      meta: { requiresAuth: true },
    },
  ],
})

// 路由守卫：未登录重定向到 /login
router.beforeEach((to, _from, next) => {
  if (to.meta.requiresAuth && !isLoggedIn()) {
    next({ name: 'Login', query: { redirect: to.fullPath } })
  } else if (to.meta.guest && isLoggedIn()) {
    next({ name: 'Products' })
  } else {
    next()
  }
})

export default router
