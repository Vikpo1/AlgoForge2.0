<template>
  <div class="page">
    <section class="room-card">
      <div class="header">
        <div>
          <h1>刷题</h1>
          <p>先看题面独立思考，再按需展开标签、贴士和笔记。</p>
        </div>

        <el-button type="primary" :loading="loading" @click="startReview">
          {{ problem ? '重新获取题目' : '获取题目' }}
        </el-button>
      </div>

      <div class="toolbar">
        <el-radio-group v-model="drawMode">
          <el-radio-button value="specific">指定题目</el-radio-button>
          <el-radio-button value="random">随机</el-radio-button>
        </el-radio-group>

        <template v-if="drawMode === 'specific'">
          <el-select
            v-model="selectedListId"
            class="scope-select"
            placeholder="选择题单"
            @change="loadProblemsForSelectedList"
          >
            <el-option
              v-for="list in lists"
              :key="list.id"
              :label="list.name"
              :value="list.id"
            />
          </el-select>

          <el-select
            v-model="selectedProblemId"
            class="problem-select"
            placeholder="选择题目"
            filterable
            :disabled="!selectedListId || listProblems.length === 0"
          >
            <el-option
              v-for="item in listProblems"
              :key="item.problem.id"
              :label="item.problem.title"
              :value="item.problem.id"
            >
              <span>{{ item.problem.title }}</span>
              <span class="option-meta">{{ item.problem.oj }}</span>
            </el-option>
          </el-select>
        </template>
      </div>

      <el-empty
        v-if="!problem && !loading"
        description="选择刷题方式后点击获取题目。"
      />

      <el-alert
        v-if="errorMessage"
        class="message"
        type="error"
        :title="errorMessage"
        show-icon
      />

      <div v-if="problem" class="review-content">
        <el-steps :active="currentStep" finish-status="success" class="steps">
          <el-step title="获取题目" />
          <el-step title="独立思考" />
          <el-step title="展开复盘" />
        </el-steps>

        <ProblemPanel
          :problem="problem"
          :stage="stage"
          :show-debug-info="showDebugInfo"
          :show-tags="showTags"
          @finish-thinking="finishThinking"
          @toggle-debug="showDebugInfo = !showDebugInfo"
          @toggle-tags="showTags = !showTags"
        />

        <section v-if="stage === 'solving'" class="spoiler-warning">
          <el-alert
            type="warning"
            show-icon
            title="防剧透模式已开启"
            description="当前只展示题面与样例，标签、贴士和完整笔记都由你主动展开。"
          />
        </section>

        <section v-if="stage === 'reviewing'" class="review-section">
          <el-alert
            type="success"
            show-icon
            title="已进入复盘阶段"
            description="现在可以展开、编辑并保存贴士和完整笔记。"
          />

          <ReviewMaterialPanel
            :loading="materialLoading"
            :error-message="materialErrorMessage"
            :material="reviewMaterial"
            :show-hint="showHint"
            :show-note="showNote"
            :saving="materialSaving"
            @toggle-hint="showHint = !showHint"
            @toggle-note="showNote = !showNote"
            @save-material="saveReviewMaterial"
          />

          <FeedbackPanel
            :selected-feedback="selectedFeedback"
            :feedback-submitting="feedbackSubmitting"
            :feedback-error-message="feedbackErrorMessage"
            :feedback-result="feedbackResult"
            @select-feedback="selectFeedback"
          />
        </section>

        <DebugSchedulePanel
          :show="showDebugInfo"
          :selected-data="selectedData"
          :review-state="reviewState"
        />
      </div>
    </section>
  </div>
</template>

<script setup>
import { computed, onMounted, ref } from 'vue'
import { ElMessage } from 'element-plus'
import { getNextReviewProblem } from '../api/devReviewApi'
import { getProblemDetail } from '../api/problemApi'
import { getProblemLists, getProblemsInList } from '../api/problemListApi'
import { getReviewMaterial, updateReviewMaterial } from '../api/reviewMaterialApi'
import { submitReviewFeedback } from '../api/reviewFeedbackApi'
import ProblemPanel from '../components/review/ProblemPanel.vue'
import ReviewMaterialPanel from '../components/review/ReviewMaterialPanel.vue'
import FeedbackPanel from '../components/review/FeedbackPanel.vue'
import DebugSchedulePanel from '../components/review/DebugSchedulePanel.vue'

const loading = ref(false)
const errorMessage = ref('')
const selectedData = ref(null)
const stage = ref('idle')
const showDebugInfo = ref(false)
const showTags = ref(false)

const drawMode = ref('specific')
const selectedListId = ref(null)
const selectedProblemId = ref(null)
const lists = ref([])
const listProblems = ref([])

const materialLoading = ref(false)
const materialSaving = ref(false)
const materialErrorMessage = ref('')
const reviewMaterial = ref(null)
const showHint = ref(false)
const showNote = ref(false)

const selectedFeedback = ref('')
const feedbackSubmitting = ref(false)
const feedbackErrorMessage = ref('')
const feedbackResult = ref(null)
const reviewStartTime = ref(0)

const problem = computed(() => selectedData.value?.problem || null)
const reviewState = computed(() => selectedData.value?.reviewState || {})

const currentStep = computed(() => {
  if (stage.value === 'solving') {
    return 1
  }

  if (stage.value === 'reviewing') {
    return 2
  }

  return 0
})

const loadLists = async () => {
  try {
    const response = await getProblemLists()
    lists.value = response.data.data.lists || []

    if (!selectedListId.value && lists.value.length) {
      selectedListId.value = lists.value[0].id
      await loadProblemsForSelectedList()
    }
  } catch (error) {
    ElMessage.error('题单加载失败')
  }
}

const loadProblemsForSelectedList = async () => {
  selectedProblemId.value = null
  listProblems.value = []

  if (!selectedListId.value) {
    return
  }

  try {
    const response = await getProblemsInList(selectedListId.value)
    listProblems.value = response.data.data.problems || []
    if (listProblems.value.length) {
      selectedProblemId.value = listProblems.value[0].problem.id
    }
  } catch (error) {
    ElMessage.error('题目加载失败')
  }
}

const resetReviewMaterial = () => {
  materialLoading.value = false
  materialSaving.value = false
  materialErrorMessage.value = ''
  reviewMaterial.value = null
  showHint.value = false
  showNote.value = false
}

const resetFeedback = () => {
  selectedFeedback.value = ''
  feedbackSubmitting.value = false
  feedbackErrorMessage.value = ''
  feedbackResult.value = null
  reviewStartTime.value = 0
}

const resetProblemState = () => {
  errorMessage.value = ''
  selectedData.value = null
  stage.value = 'idle'
  showDebugInfo.value = false
  showTags.value = false
  resetReviewMaterial()
  resetFeedback()
}

const startReview = async () => {
  loading.value = true
  resetProblemState()

  try {
    let response
    if (drawMode.value === 'specific') {
      if (!selectedProblemId.value) {
        errorMessage.value = '请先选择一个具体题目'
        return
      }
      response = await getProblemDetail(selectedProblemId.value)
    } else {
      response = await getNextReviewProblem({ mode: 'random' })
    }

    if (response.data.code === 0) {
      selectedData.value = response.data.data.candidate || response.data.data
      stage.value = 'solving'
      reviewStartTime.value = Date.now()
    } else {
      errorMessage.value = response.data.message || '获取题目失败'
    }
  } catch (error) {
    errorMessage.value = '无法连接后端，请确认 C++ 服务是否正在运行'
  } finally {
    loading.value = false
  }
}

const finishThinking = async () => {
  if (!problem.value?.id) {
    return
  }

  stage.value = 'reviewing'
  resetReviewMaterial()
  materialLoading.value = true

  try {
    const response = await getReviewMaterial(problem.value.id)

    if (response.data.code === 0) {
      reviewMaterial.value = response.data.data
    } else {
      materialErrorMessage.value = response.data.message || '获取复盘材料失败'
    }
  } catch (error) {
    materialErrorMessage.value = '无法获取复盘材料，请确认后端接口是否正常'
  } finally {
    materialLoading.value = false
  }
}

const saveReviewMaterial = async (payload) => {
  if (!problem.value?.id) {
    return
  }

  materialSaving.value = true
  materialErrorMessage.value = ''

  try {
    const response = await updateReviewMaterial(problem.value.id, payload)
    if (response.data.code === 0) {
      reviewMaterial.value = response.data.data
      ElMessage.success('复盘材料已保存')
    } else {
      materialErrorMessage.value = response.data.message || '保存复盘材料失败'
    }
  } catch (error) {
    materialErrorMessage.value = '保存复盘材料失败，请确认后端接口是否正常'
  } finally {
    materialSaving.value = false
  }
}

const selectFeedback = async (feedback) => {
  if (!problem.value?.id || feedbackResult.value) {
    return
  }

  selectedFeedback.value = feedback
  feedbackSubmitting.value = true
  feedbackErrorMessage.value = ''
  feedbackResult.value = null

  const durationSeconds = reviewStartTime.value
    ? Math.floor((Date.now() - reviewStartTime.value) / 1000)
    : 0

  try {
    const response = await submitReviewFeedback(problem.value.id, feedback, durationSeconds)

    if (response.data.code === 0) {
      feedbackResult.value = response.data.data
    } else {
      feedbackErrorMessage.value = response.data.message || '提交反馈失败'
    }
  } catch (error) {
    feedbackErrorMessage.value = '无法提交反馈，请确认后端接口是否正常'
  } finally {
    feedbackSubmitting.value = false
  }
}

onMounted(loadLists)
</script>

<style scoped>
.page {
  min-height: calc(100vh - 64px);
  box-sizing: border-box;
  padding: 32px;
}

.room-card {
  max-width: 1100px;
  margin: 0 auto;
  padding: 28px;
  box-sizing: border-box;
  background: #ffffff;
  border: 1px solid #ebeef5;
  border-radius: 8px;
}

.header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 24px;
  margin-bottom: 22px;
}

.header h1 {
  margin: 0;
  font-size: 26px;
}

.header p {
  margin: 8px 0 0;
  color: #606266;
}

.toolbar {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 16px;
  margin-bottom: 22px;
}

.scope-select {
  width: 220px;
}

.problem-select {
  width: min(480px, 100%);
}

.option-meta {
  float: right;
  color: #909399;
  font-size: 12px;
}

.message {
  margin-bottom: 20px;
}

.review-content {
  margin-top: 8px;
}

.steps {
  margin-bottom: 28px;
}

.spoiler-warning {
  margin-top: 20px;
}

.review-section {
  margin-top: 20px;
}

@media (max-width: 760px) {
  .header {
    flex-direction: column;
  }

  .scope-select,
  .problem-select {
    width: 100%;
  }
}
</style>
