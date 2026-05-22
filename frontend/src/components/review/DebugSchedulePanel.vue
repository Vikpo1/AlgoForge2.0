<template>
  <el-collapse v-if="show" class="debug-panel">
    <el-collapse-item title="开发调试信息：当前题目的调度状态" name="debug">
      <el-descriptions :column="1" border>
        <el-descriptions-item label="所属题单 ID">
          {{ selectedData.listId }}
        </el-descriptions-item>

        <el-descriptions-item label="复习状态">
          {{ getReviewStatusText(reviewState.status) }}
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
          {{ getFeedbackText(reviewState.lastFeedback) }}
        </el-descriptions-item>
      </el-descriptions>
    </el-collapse-item>
  </el-collapse>
</template>

<script setup>
import { getFeedbackText, getReviewStatusText } from '../../utils/reviewText'

defineProps({
  show: {
    type: Boolean,
    default: false
  },
  selectedData: {
    type: Object,
    required: true
  },
  reviewState: {
    type: Object,
    default: () => ({})
  }
})
</script>

<style scoped>
.debug-panel {
  margin-top: 24px;
}
</style>