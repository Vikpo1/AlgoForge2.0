<template>
  <div class="app-shell">
    <header v-if="$route.path !== '/login'" class="app-header">
      <div class="brand">
        <strong>AlgoForge</strong>
        <span>智能算法复习平台</span>
      </div>

      <el-menu
        class="nav"
        mode="horizontal"
        router
        :ellipsis="false"
        :default-active="$route.path"
      >
        <el-menu-item index="/">主菜单</el-menu-item>
        <el-menu-item index="/lists">题单管理</el-menu-item>
        <el-menu-item index="/review">刷题</el-menu-item>
        <el-menu-item index="/stats">统计</el-menu-item>
      </el-menu>

      <div class="account">
        <span>{{ user?.username || '未登录' }}</span>
        <el-button size="small" plain @click="logout">退出</el-button>
      </div>
    </header>

    <RouterView />
  </div>
</template>

<script setup>
import { onMounted, onUnmounted, ref } from 'vue'
import { RouterView, useRouter } from 'vue-router'
import { clearAuthSession, getAuthUser } from './utils/auth'

const router = useRouter()
const user = ref(getAuthUser())

const syncUser = () => {
  user.value = getAuthUser()
}

const logout = () => {
  clearAuthSession()
  router.push('/login')
}

onMounted(() => {
  window.addEventListener('algoforge-auth-changed', syncUser)
})

onUnmounted(() => {
  window.removeEventListener('algoforge-auth-changed', syncUser)
})
</script>

<style>
html,
body,
#app {
  margin: 0;
  min-height: 100vh;
}

body {
  background: #f5f7fa;
}

.app-shell {
  min-height: 100vh;
}

.app-header {
  position: sticky;
  top: 0;
  z-index: 10;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 24px;
  height: 64px;
  padding: 0 32px;
  box-sizing: border-box;
  background: #ffffff;
  border-bottom: 1px solid #ebeef5;
}

.brand {
  display: flex;
  align-items: baseline;
  gap: 12px;
  white-space: nowrap;
}

.brand strong {
  font-size: 20px;
}

.brand span {
  color: #606266;
  font-size: 13px;
}

.nav {
  flex: 1;
  justify-content: flex-end;
  border-bottom: none;
}

.account {
  display: flex;
  align-items: center;
  gap: 10px;
  color: #606266;
  white-space: nowrap;
}

@media (max-width: 860px) {
  .app-header {
    height: auto;
    flex-wrap: wrap;
    padding: 12px 16px;
  }
}
</style>
