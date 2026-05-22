<template>
  <div class="app-shell">
    <header class="app-header">
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
      </el-menu>
    </header>

    <RouterView />
  </div>
</template>

<script setup>
import { RouterView } from 'vue-router'
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
</style>
