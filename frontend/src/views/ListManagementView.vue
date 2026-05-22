<template>
  <main class="page">
    <section class="panel">
      <div class="panel-header">
        <div>
          <h1>题单管理</h1>
          <p>创建题单，维护题单说明和权值，也可以调整题单内每道题的抽取权值。</p>
        </div>

        <el-button type="primary" plain @click="loadLists">刷新</el-button>
      </div>

      <el-form class="create-form" label-position="top">
        <div class="create-grid">
          <el-form-item label="题单名称">
            <el-input v-model="newList.name" placeholder="例如：赛后补题" />
          </el-form-item>

          <el-form-item label="权值">
            <el-input-number v-model="newList.listUserWeight" :min="0" :max="100" />
          </el-form-item>
        </div>

        <el-form-item label="题单描述">
          <el-input
            v-model="newList.description"
            type="textarea"
            :rows="2"
            placeholder="例如：记录 ABC 赛后需要二刷的题目"
          />
        </el-form-item>

        <el-button type="primary" :loading="creating" @click="createList">创建题单</el-button>
      </el-form>

      <el-table
        class="list-table"
        :data="lists"
        border
        highlight-current-row
        @current-change="selectList"
      >
        <el-table-column label="名称" min-width="180">
          <template #default="{ row }">
            <el-input
              v-model="row.name"
              @blur="scheduleListSave(row)"
              @change="scheduleListSave(row)"
            />
          </template>
        </el-table-column>

        <el-table-column label="描述" min-width="260">
          <template #default="{ row }">
            <el-input
              v-model="row.description"
              type="textarea"
              :rows="2"
              @blur="scheduleListSave(row)"
              @change="scheduleListSave(row)"
            />
          </template>
        </el-table-column>

        <el-table-column label="权值" width="170">
          <template #default="{ row }">
            <el-input-number
              v-model="row.listUserWeight"
              :min="0"
              :max="100"
              @change="scheduleListSave(row)"
            />
          </template>
        </el-table-column>

        <el-table-column prop="problemCount" label="题目数" width="90" />

        <el-table-column label="操作" width="110">
          <template #default="{ row }">
            <el-button size="small" type="danger" plain @click.stop="removeList(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
    </section>

    <section class="panel detail-panel">
      <div class="panel-header compact">
        <div>
          <h2>{{ selectedList ? selectedList.name : '题单内题目' }}</h2>
          <p>选择上方题单后，可修改其中每道题的用户权值。</p>
        </div>
      </div>

      <el-empty v-if="!selectedList" description="请先选择一个题单" />

      <el-table v-else :data="problems" border>
        <el-table-column label="题目" min-width="260">
          <template #default="{ row }">
            <div class="problem-title">{{ row.problem.title }}</div>
            <div class="problem-meta">{{ row.problem.oj }} · {{ row.problem.difficulty }}</div>
          </template>
        </el-table-column>

        <el-table-column label="状态" width="140">
          <template #default="{ row }">
            {{ getReviewStatusText(row.reviewState.status) }}
          </template>
        </el-table-column>

        <el-table-column label="题目权值" width="190">
          <template #default="{ row }">
            <el-input-number
              v-model="row.reviewState.problemUserWeight"
              :min="0"
              :max="100"
              @change="scheduleProblemWeightSave(row)"
            />
          </template>
        </el-table-column>
      </el-table>
    </section>
  </main>
</template>

<script setup>
import { onBeforeUnmount, onMounted, reactive, ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  createProblemList,
  deleteProblemList,
  getProblemLists,
  getProblemsInList,
  updateProblemList
} from '../api/problemListApi'
import { updateProblemWeight } from '../api/problemApi'
import { getReviewStatusText } from '../utils/reviewText'

const lists = ref([])
const problems = ref([])
const selectedList = ref(null)
const creating = ref(false)

const listSaveTimers = new Map()
const problemSaveTimers = new Map()

const newList = reactive({
  name: '',
  description: '',
  listUserWeight: 50
})

const loadLists = async () => {
  try {
    const response = await getProblemLists()
    lists.value = response.data.data.lists || []

    if (selectedList.value) {
      selectedList.value = lists.value.find(list => list.id === selectedList.value.id) || null
    }
  } catch (error) {
    ElMessage.error('题单加载失败')
  }
}

const selectList = async (row) => {
  selectedList.value = row
  problems.value = []

  if (!row) {
    return
  }

  try {
    const response = await getProblemsInList(row.id)
    problems.value = response.data.data.problems || []
  } catch (error) {
    ElMessage.error('题单内题目加载失败')
  }
}

const createList = async () => {
  if (!newList.name.trim()) {
    ElMessage.warning('请填写题单名称')
    return
  }

  creating.value = true

  try {
    await createProblemList({
      name: newList.name,
      description: newList.description,
      listUserWeight: newList.listUserWeight
    })
    newList.name = ''
    newList.description = ''
    newList.listUserWeight = 50
    await loadLists()
    ElMessage.success('题单已创建')
  } catch (error) {
    ElMessage.error('创建题单失败')
  } finally {
    creating.value = false
  }
}

const saveList = async (row) => {
  if (!row.name.trim()) {
    ElMessage.warning('题单名称不能为空')
    return
  }

  try {
    await updateProblemList(row.id, {
      name: row.name,
      description: row.description || '',
      listUserWeight: row.listUserWeight
    })
  } catch (error) {
    ElMessage.error('保存题单失败')
  }
}

const scheduleListSave = (row) => {
  window.clearTimeout(listSaveTimers.get(row.id))
  listSaveTimers.set(row.id, window.setTimeout(() => saveList(row), 450))
}

const removeList = async (row) => {
  try {
    await ElMessageBox.confirm(
      `删除「${row.name}」会一并移除其中不再属于其他题单的题目，是否继续？`,
      '删除题单',
      { type: 'warning' }
    )

    await deleteProblemList(row.id)
    if (selectedList.value?.id === row.id) {
      selectedList.value = null
      problems.value = []
    }
    await loadLists()
    ElMessage.success('题单已删除')
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除题单失败')
    }
  }
}

const saveProblemWeight = async (row) => {
  try {
    await updateProblemWeight(row.problem.id, row.reviewState.problemUserWeight)
  } catch (error) {
    ElMessage.error('保存题目权值失败')
  }
}

const scheduleProblemWeightSave = (row) => {
  const problemId = row.problem.id
  window.clearTimeout(problemSaveTimers.get(problemId))
  problemSaveTimers.set(problemId, window.setTimeout(() => saveProblemWeight(row), 350))
}

onMounted(loadLists)

onBeforeUnmount(() => {
  listSaveTimers.forEach(timer => window.clearTimeout(timer))
  problemSaveTimers.forEach(timer => window.clearTimeout(timer))
})
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

.detail-panel {
  margin-top: 24px;
}

.panel-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 24px;
  margin-bottom: 24px;
}

.panel-header.compact {
  margin-bottom: 18px;
}

h1,
h2 {
  margin: 0;
}

h1 {
  font-size: 26px;
}

h2 {
  font-size: 20px;
}

p {
  margin: 8px 0 0;
  color: #606266;
}

.create-form {
  margin-bottom: 18px;
}

.create-grid {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 180px;
  gap: 16px;
}

.problem-title {
  font-weight: 600;
}

.problem-meta {
  margin-top: 4px;
  color: #909399;
  font-size: 13px;
}

@media (max-width: 760px) {
  .create-grid {
    grid-template-columns: 1fr;
  }
}
</style>
