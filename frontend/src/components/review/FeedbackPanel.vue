<template>
  <el-card class="feedback-card" shadow="never">
    <template #header>
      <div class="feedback-header">
        <span>掌握度反馈</span>
        <el-tag type="success" effect="plain">提交后端</el-tag>
      </div>
    </template>

    <p class="feedback-description">
      根据本次复习表现选择掌握程度。反馈会提交给后端，用于更新题目的状态、冷却时间和下一次抽取权重。
    </p>

    <div class="feedback-actions">
      <el-button
        type="danger"
        plain
        :disabled="!!feedbackResult"
        :loading="feedbackSubmitting && selectedFeedback === 'FAILED'"
        @click="$emit('select-feedback', 'FAILED')"
      >
        完全不会
      </el-button>

      <el-button
        type="warning"
        plain
        :disabled="!!feedbackResult"
        :loading="feedbackSubmitting && selectedFeedback === 'SOLVED_WITH_HINT'"
        @click="$emit('select-feedback', 'SOLVED_WITH_HINT')"
      >
        看提示才会
      </el-button>

      <el-button
        type="primary"
        plain
        :disabled="!!feedbackResult"
        :loading="feedbackSubmitting && selectedFeedback === 'SOLVED_SLOWLY'"
        @click="$emit('select-feedback', 'SOLVED_SLOWLY')"
      >
        独立做出但较慢
      </el-button>

      <el-button
        type="success"
        plain
        :disabled="!!feedbackResult"
        :loading="feedbackSubmitting && selectedFeedback === 'SOLVED_FAST'"
        @click="$emit('select-feedback', 'SOLVED_FAST')"
      >
        秒杀
      </el-button>
    </div>

    <el-alert
      v-if="feedbackErrorMessage"
      class="feedback-result"
      type="error"
      show-icon
      :title="feedbackErrorMessage"
    />

    <el-alert
      v-if="feedbackResult"
      class="feedback-result"
      type="success"
      show-icon
      :title="`反馈已提交：${getFeedbackText(feedbackResult.submittedFeedback)}`"
    />

    <el-descriptions v-if="feedbackResult" class="feedback-state" :column="1" border>
      <el-descriptions-item label="更新后状态">
        {{ getReviewStatusText(feedbackResult.updatedReviewState.status) }}
      </el-descriptions-item>

      <el-descriptions-item label="下一次复习间隔">
        {{ feedbackResult.nextReviewDelayHours }} 小时
      </el-descriptions-item>

      <el-descriptions-item label="本次耗时">
        {{ feedbackResult.durationSeconds }} 秒
      </el-descriptions-item>

      <el-descriptions-item label="新的状态权重">
        {{ feedbackResult.updatedReviewState.statusWeight }}
      </el-descriptions-item>

      <el-descriptions-item label="新的调度权重">
        {{ feedbackResult.updatedReviewState.scheduleWeight }}
      </el-descriptions-item>
    </el-descriptions>
  </el-card>
</template>

<script setup>
import { getFeedbackText, getReviewStatusText } from '../../utils/reviewText'

defineProps({
  selectedFeedback: {
    type: String,
    default: ''
  },
  feedbackSubmitting: {
    type: Boolean,
    default: false
  },
  feedbackErrorMessage: {
    type: String,
    default: ''
  },
  feedbackResult: {
    type: Object,
    default: null
  }
})

defineEmits(['select-feedback'])
</script>

<style scoped>
.feedback-card {
  margin-top: 16px;
}

.feedback-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.feedback-description {
  margin: 0 0 20px;
  color: #606266;
  line-height: 1.8;
}

.feedback-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
}

.feedback-result {
  margin-top: 20px;
}

.feedback-state {
  margin-top: 20px;
}
</style>