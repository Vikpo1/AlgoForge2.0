import { createRouter, createWebHistory } from 'vue-router'
import DevHealthView from '../views/DevHealthView.vue'
import DevReviewView from '../views/DevReviewView.vue'
import LoginView from '../views/LoginView.vue'
import ListManagementView from '../views/ListManagementView.vue'
import MainMenuView from '../views/MainMenuView.vue'
import ReviewRoomView from '../views/ReviewRoomView.vue'
import StatsView from '../views/StatsView.vue'
import { isAuthenticated } from '../utils/auth'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/login',
      name: 'login',
      component: LoginView,
      meta: { guestOnly: true }
    },
    {
      path: '/',
      name: 'main-menu',
      component: MainMenuView,
      meta: { requiresAuth: true }
    },
    {
      path: '/lists',
      name: 'list-management',
      component: ListManagementView,
      meta: { requiresAuth: true }
    },
    {
      path: '/review',
      name: 'review-room',
      component: ReviewRoomView,
      meta: { requiresAuth: true }
    },
    {
      path: '/stats',
      name: 'stats',
      component: StatsView,
      meta: { requiresAuth: true }
    },
    {
      path: '/dev/health',
      name: 'dev-health',
      component: DevHealthView
    },
    {
      path: '/dev/review',
      name: 'dev-review',
      component: DevReviewView
    }
  ]
})

router.beforeEach((to, from, next) => {
  const loggedIn = isAuthenticated()
  if (to.meta.requiresAuth && !loggedIn) {
    next('/login')
    return
  }
  if (to.meta.guestOnly && loggedIn) {
    next('/')
    return
  }
  next()
})

export default router
