import { createRouter, createWebHistory } from 'vue-router'
import DevHealthView from '../views/DevHealthView.vue'
import DevReviewView from '../views/DevReviewView.vue'
import ListManagementView from '../views/ListManagementView.vue'
import MainMenuView from '../views/MainMenuView.vue'
import ReviewRoomView from '../views/ReviewRoomView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'main-menu',
      component: MainMenuView
    },
    {
      path: '/lists',
      name: 'list-management',
      component: ListManagementView
    },
    {
      path: '/review',
      name: 'review-room',
      component: ReviewRoomView
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

export default router
