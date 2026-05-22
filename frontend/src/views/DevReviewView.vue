<template>
  <div class="page">
    <el-card class="card">
      <template #header>
        <div class="card-header">
          <span>AlgoForge 临时抽题测试</span>
        </div>
      </template>

      <p class="description">
        当前页面用于测试后端两级调度逻辑：先按题单权重抽题单，再按题目权重与状态权重抽题目。
      </p>

      <el-button type="primary" :loading="loading" @click="drawProblem">
        抽取下一道复习题
      </el-button>

      <el-alert
        v-if="errorMessage"
        class="result"
        type="error"
        :title="errorMessage"
        show-icon
      />

      <div v-if="problem" class="problem-panel">
        <h2>{{ problem.title }}</h2>

        <el-descriptions :column="1" border>
          <el-descriptions-item label="题目 ID">
            {{ problem.id }}
          </el-descriptions-item>

          <el-descriptions-item label="来源 OJ">
            {{ problem.oj }}
          </el-descriptions-item>

          <el-descriptions-item label="难度">
            {{ problem.difficulty }}
          </el-descriptions-item>

          <el-descriptions-item label="所属题单 ID">
            {{ selectedData.listId }}
          </el-descriptions-item>

          <el-descriptions-item label="复习状态">
            {{ reviewState.status }}
          </el-descriptions-item>

          <el-descriptions-item label="题目用户权重">
            {{ reviewState.problemUserWeight }}
          </el-descriptions-item>

          <el-descriptions-item label="状态权重">
            {{ reviewState.statusWeight }}
          </el-descriptions-item>

          <el-descriptions-item label="最终调度权重">
            {{ reviewState.scheduleWeight }}
          </el-descriptions-item>

          <el-descriptions-item label="复习次数">
            {{ reviewState.reviewCount }}
          </el-descriptions-item>

          <el-descriptions-item label="上次反馈">
            {{ reviewState.lastFeedback }}
          </el-descriptions-item>
        </el-descriptions>

        <el-button class="open-button" type="success" @click="openProblem">
          打开原题链接
        </el-button>
      </div>

      <pre v-if="responseText" class="response">{{ responseText }}</pre>
    </el-card>
  </div>
</template>

<script setup>
import { computed, ref } from 'vue'
import { getNextReviewProblem } from '../api/devReviewApi'

const loading = ref(false)
const errorMessage = ref('')
const responseText = ref('')
const selectedData = ref(null)

const problem = computed(() => selectedData.value?.problem || null)
const reviewState = computed(() => selectedData.value?.reviewState || {})

const drawProblem = async () => {
  loading.value = true
  errorMessage.value = ''
  responseText.value = ''
  selectedData.value = null

  try {
    const response = await getNextReviewProblem()

    responseText.value = JSON.stringify(response.data, null, 2)

    if (response.data.code === 0) {
      selectedData.value = response.data.data
    } else {
      errorMessage.value = response.data.message || '抽题失败'
    }
  } catch (error) {
    errorMessage.value = '无法连接后端，请确认 C++ 服务是否正在运行'
    responseText.value = String(error)
  } finally {
    loading.value = false
  }
}

const openProblem = () => {
  if (problem.value?.url) {
    window.open(problem.value.url, '_blank')
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
  max-width: 900px;
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

.problem-panel {
  margin-top: 28px;
}

.problem-panel h2 {
  margin: 0 0 20px;
  font-size: 24px;
}

.open-button {
  margin-top: 20px;
}

.response {
  margin-top: 24px;
  padding: 16px;
  border-radius: 8px;
  background: #1f2937;
  color: #e5e7eb;
  overflow-x: auto;
}
</style>