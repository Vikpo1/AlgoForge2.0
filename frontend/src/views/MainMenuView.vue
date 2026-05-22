<template>
  <main class="page">
    <section class="panel">
      <div class="panel-header">
        <div>
          <h1>主菜单</h1>
          <p>粘贴外部 OJ 题目链接，并把它加入一个已有题单。</p>
        </div>

        <el-button type="primary" plain @click="loadLists">刷新题单</el-button>
      </div>

      <el-alert
        v-if="lists.length === 0 && !loading"
        class="notice"
        type="warning"
        show-icon
        title="当前没有可用题单，请先进入题单管理创建题单。"
      />

      <el-form class="import-form" label-position="top">
        <el-form-item label="题目链接">
          <el-input
            v-model="form.url"
            placeholder="https://codeforces.com/problemset/problem/..."
            clearable
          />
        </el-form-item>

        <el-form-item label="目标题单">
          <el-select
            v-model="form.listId"
            class="full"
            placeholder="选择要加入的题单"
            :disabled="lists.length === 0"
          >
            <el-option
              v-for="list in lists"
              :key="list.id"
              :label="`${list.name}（权值 ${list.listUserWeight}）`"
              :value="list.id"
            />
          </el-select>
        </el-form-item>

        <div class="actions">
          <el-button
            type="primary"
            :loading="submitting"
            :disabled="!form.url || !form.listId"
            @click="submitImport"
          >
            添加题目
          </el-button>

          <el-button @click="$router.push('/lists')">进入题单管理</el-button>
          <el-button @click="$router.push('/review')">进入刷题</el-button>
        </div>
      </el-form>

      <el-alert
        v-if="lastImported"
        class="result"
        type="success"
        show-icon
        :title="`已添加：${lastImported.problem.title}`"
      />

      <el-table v-if="lists.length" class="list-table" :data="lists" border>
        <el-table-column prop="name" label="已有题单" min-width="160" />
        <el-table-column prop="description" label="描述" min-width="220" />
        <el-table-column prop="problemCount" label="题目数" width="100" />
        <el-table-column prop="listUserWeight" label="题单权值" width="120" />
      </el-table>
    </section>
  </main>
</template>

<script setup>
import { onMounted, reactive, ref } from 'vue'
import { ElMessage } from 'element-plus'
import { getProblemLists } from '../api/problemListApi'
import { importProblem } from '../api/problemApi'

const loading = ref(false)
const submitting = ref(false)
const lists = ref([])
const lastImported = ref(null)

const form = reactive({
  url: '',
  listId: null
})

const loadLists = async () => {
  loading.value = true

  try {
    const response = await getProblemLists()
    lists.value = response.data.data.lists || []

    if (!form.listId && lists.value.length) {
      form.listId = lists.value[0].id
    }
  } catch (error) {
    ElMessage.error('无法加载题单，请确认后端服务是否运行')
  } finally {
    loading.value = false
  }
}

const submitImport = async () => {
  submitting.value = true
  lastImported.value = null

  try {
    const response = await importProblem({
      url: form.url,
      listId: form.listId
    })

    lastImported.value = response.data.data.candidate
    form.url = ''
    await loadLists()
    ElMessage.success('题目已加入题单')
  } catch (error) {
    ElMessage.error(error.response?.data?.message || '添加失败，请检查链接和目标题单')
  } finally {
    submitting.value = false
  }
}

onMounted(loadLists)
</script>

<style scoped>
.page {
  box-sizing: border-box;
  min-height: calc(100vh - 64px);
  padding: 32px;
}

.panel {
  max-width: 960px;
  margin: 0 auto;
  padding: 28px;
  box-sizing: border-box;
  background: #ffffff;
  border: 1px solid #ebeef5;
  border-radius: 8px;
}

.panel-header {
  display: flex;
  align-items: flex-start;
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

.notice,
.result {
  margin-bottom: 20px;
}

.import-form {
  max-width: 720px;
}

.full {
  width: 100%;
}

.actions {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
}

.list-table {
  margin-top: 28px;
}
</style>
