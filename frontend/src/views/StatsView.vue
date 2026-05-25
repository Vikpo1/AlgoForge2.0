<template>
  <main class="page">
    <section class="panel">
      <div class="panel-header">
        <div>
          <h1>刷题统计</h1>
          <p>点击任意反馈按钮即计入当天完成题数；同一题同一天只统计一次。</p>
        </div>
        <el-button type="primary" plain :loading="loading" @click="loadStats">刷新</el-button>
      </div>

      <div class="summary-grid">
        <div class="summary-item">
          <span>最近一年完成</span>
          <strong>{{ totalCount }}</strong>
        </div>
        <div class="summary-item">
          <span>活跃天数</span>
          <strong>{{ activeDays }}</strong>
        </div>
        <div class="summary-item">
          <span>今日完成</span>
          <strong>{{ todayCount }}</strong>
        </div>
      </div>

      <el-alert
        v-if="errorMessage"
        class="message"
        type="error"
        :title="errorMessage"
        show-icon
      />

      <div class="heatmap-wrap">
        <div class="month-row">
          <span
            v-for="month in monthLabels"
            :key="month.key"
            class="month-label"
            :style="{ gridColumnStart: month.week + 1 }"
          >
            {{ month.label }}
          </span>
        </div>

        <div class="heatmap-body">
          <div class="weekday-labels">
            <span></span>
            <span>一</span>
            <span></span>
            <span>三</span>
            <span></span>
            <span>五</span>
            <span></span>
          </div>

          <div class="heatmap-grid">
            <div
              v-for="day in heatmapDays"
              :key="day.date"
              class="day-cell"
              :class="levelClass(day.count)"
              @mouseenter="scheduleTooltip(day, $event)"
              @mousemove="moveTooltip($event)"
              @mouseleave="hideTooltip"
            />
          </div>
        </div>

        <div
          v-if="hoveredTip"
          class="custom-tooltip"
          :style="tooltipStyle"
        >
          {{ hoveredTip.date }}：{{ hoveredTip.count }} 题
        </div>

        <div class="legend">
          <span>少</span>
          <i class="day-cell level-0"></i>
          <i class="day-cell level-1"></i>
          <i class="day-cell level-2"></i>
          <i class="day-cell level-3"></i>
          <i class="day-cell level-4"></i>
          <span>多</span>
        </div>
      </div>
    </section>
  </main>
</template>

<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { getDailyActivity } from '../api/statsApi'

const loading = ref(false)
const errorMessage = ref('')
const stats = ref([])
const totalCount = ref(0)
const activeDays = ref(0)
const hoveredTip = ref(null)
const tooltipStyle = ref({})
let tooltipTimer = null

const dateKey = (date) => {
  const year = date.getFullYear()
  const month = String(date.getMonth() + 1).padStart(2, '0')
  const day = String(date.getDate()).padStart(2, '0')
  return `${year}-${month}-${day}`
}

const startOfDay = (date) => new Date(date.getFullYear(), date.getMonth(), date.getDate())

const heatmapDays = computed(() => {
  const countMap = new Map(stats.value.map(item => [item.date, item.count]))
  const today = startOfDay(new Date())
  const start = new Date(today)
  start.setDate(start.getDate() - 364)

  const days = []
  const cursor = new Date(start)
  while (cursor <= today) {
    const key = dateKey(cursor)
    days.push({
      date: key,
      count: countMap.get(key) || 0
    })
    cursor.setDate(cursor.getDate() + 1)
  }
  return days
})

const monthLabels = computed(() => {
  const labels = []
  let previous = ''
  heatmapDays.value.forEach((day, index) => {
    const date = new Date(`${day.date}T00:00:00`)
    const month = date.getMonth()
    if (date.getDate() <= 7 && month !== previous) {
      labels.push({
        key: day.date,
        label: `${month + 1}月`,
        week: Math.floor(index / 7)
      })
      previous = month
    }
  })
  return labels
})

const todayCount = computed(() => {
  const today = dateKey(new Date())
  return stats.value.find(item => item.date === today)?.count || 0
})

const levelClass = (count) => {
  if (count <= 0) return 'level-0'
  if (count === 1) return 'level-1'
  if (count <= 3) return 'level-2'
  if (count <= 5) return 'level-3'
  return 'level-4'
}

const updateTooltipPosition = (event) => {
  tooltipStyle.value = {
    left: `${event.clientX + 12}px`,
    top: `${event.clientY - 36}px`
  }
}

const clearTooltipTimer = () => {
  if (tooltipTimer) {
    window.clearTimeout(tooltipTimer)
    tooltipTimer = null
  }
}

const scheduleTooltip = (day, event) => {
  clearTooltipTimer()
  updateTooltipPosition(event)
  tooltipTimer = window.setTimeout(() => {
    hoveredTip.value = day
    tooltipTimer = null
  }, 300)
}

const moveTooltip = (event) => {
  if (hoveredTip.value) {
    updateTooltipPosition(event)
  }
}

const hideTooltip = () => {
  clearTooltipTimer()
  hoveredTip.value = null
}

const loadStats = async () => {
  loading.value = true
  errorMessage.value = ''
  try {
    const response = await getDailyActivity(365)
    stats.value = response.data.data.days || []
    totalCount.value = response.data.data.totalCount || 0
    activeDays.value = response.data.data.activeDays || 0
  } catch (error) {
    errorMessage.value = '统计信息加载失败'
  } finally {
    loading.value = false
  }
}

onMounted(loadStats)
onBeforeUnmount(clearTooltipTimer)
</script>

<style scoped>
.page {
  box-sizing: border-box;
  min-height: calc(100vh - 64px);
  padding: 32px;
}

.panel {
  max-width: 1100px;
  margin: 0 auto;
  padding: 28px;
  box-sizing: border-box;
  background: #ffffff;
  border: 1px solid #ebeef5;
  border-radius: 8px;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  gap: 24px;
  margin-bottom: 24px;
}

h1 {
  margin: 0;
  font-size: 26px;
}

p {
  margin: 8px 0 0;
  color: #606266;
}

.summary-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
  margin-bottom: 24px;
}

.summary-item {
  padding: 16px;
  border: 1px solid #ebeef5;
  border-radius: 8px;
}

.summary-item span {
  display: block;
  color: #606266;
  font-size: 13px;
}

.summary-item strong {
  display: block;
  margin-top: 6px;
  font-size: 28px;
}

.message {
  margin-bottom: 18px;
}

.heatmap-wrap {
  position: relative;
  overflow-x: auto;
  padding-bottom: 6px;
}

.month-row {
  display: grid;
  grid-template-columns: repeat(53, 14px);
  gap: 4px;
  margin-left: 30px;
  min-width: 940px;
  height: 20px;
}

.month-label {
  color: #606266;
  font-size: 12px;
  white-space: nowrap;
}

.heatmap-body {
  display: flex;
  gap: 8px;
  min-width: 970px;
}

.weekday-labels {
  display: grid;
  grid-template-rows: repeat(7, 14px);
  gap: 4px;
  width: 22px;
  color: #909399;
  font-size: 12px;
}

.heatmap-grid {
  display: grid;
  grid-auto-flow: column;
  grid-template-rows: repeat(7, 14px);
  gap: 4px;
}

.day-cell {
  width: 14px;
  height: 14px;
  border-radius: 3px;
  border: 1px solid rgba(27, 31, 36, 0.06);
}

.level-0 {
  background: #ebedf0;
}

.level-1 {
  background: #9be9a8;
}

.level-2 {
  background: #40c463;
}

.level-3 {
  background: #30a14e;
}

.level-4 {
  background: #216e39;
}

.legend {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 6px;
  min-width: 970px;
  margin-top: 14px;
  color: #606266;
  font-size: 12px;
}

.custom-tooltip {
  position: fixed;
  z-index: 2000;
  padding: 7px 10px;
  color: #ffffff;
  font-size: 12px;
  line-height: 1;
  white-space: nowrap;
  pointer-events: none;
  background: #303133;
  border-radius: 4px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.18);
}

@media (max-width: 760px) {
  .panel-header,
  .summary-grid {
    grid-template-columns: 1fr;
    flex-direction: column;
  }
}
</style>
