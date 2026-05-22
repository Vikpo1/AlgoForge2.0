<template>
  <div class="page">
    <el-card class="card">
      <template #header>
        <div class="card-header">
          <span>AlgoForge 前后端联通测试</span>
        </div>
      </template>

      <p class="description">
        当前页面用于测试 Vue 前端是否能正常访问 C++ 后端健康检查接口。
      </p>

      <el-button type="primary" :loading="loading" @click="checkHealth">
        检查后端状态
      </el-button>

      <el-alert
        v-if="successMessage"
        class="result"
        type="success"
        :title="successMessage"
        show-icon
      />

      <el-alert
        v-if="errorMessage"
        class="result"
        type="error"
        :title="errorMessage"
        show-icon
      />

      <pre v-if="responseText" class="response">{{ responseText }}</pre>
    </el-card>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { getHealth } from '../api/healthApi'

const loading = ref(false)
const successMessage = ref('')
const errorMessage = ref('')
const responseText = ref('')

const checkHealth = async () => {
  loading.value = true
  successMessage.value = ''
  errorMessage.value = ''
  responseText.value = ''

  try {
    const response = await getHealth()

    responseText.value = JSON.stringify(response.data, null, 2)

    if (response.data.code === 0) {
      successMessage.value = '后端连接正常'
    } else {
      errorMessage.value = '后端返回了异常状态'
    }
  } catch (error) {
    errorMessage.value = '无法连接后端，请确认 C++ 服务是否正在运行'
    responseText.value = String(error)
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.page {
  min-height: 100vh;
  box-sizing: border-box;
  padding: 48px;
  background: #f5f7fa;
}

.card {
  max-width: 720px;
  margin: 0 auto;
}

.card-header {
  font-size: 18px;
  font-weight: 600;
}

.description {
  margin-bottom: 24px;
  color: #606266;
  line-height: 1.7;
}

.result {
  margin-top: 20px;
}

.response {
  margin-top: 20px;
  padding: 16px;
  border-radius: 8px;
  background: #1f2937;
  color: #e5e7eb;
  overflow-x: auto;
}
</style>