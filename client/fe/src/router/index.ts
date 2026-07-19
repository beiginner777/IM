import { createRouter, createWebHistory } from 'vue-router'
import { isLoggedIn } from '../utils/token'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      name: 'Home',
      component: () => import('../pages/HomePage.vue'),
    },
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
