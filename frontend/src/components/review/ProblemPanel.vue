<template>
  <section class="problem-section">
    <div class="problem-title-row">
      <div>
        <h2>{{ problem.title }}</h2>
        <div class="problem-meta">{{ problem.oj }} · {{ problem.difficulty }}</div>
      </div>

      <el-tag type="info" effect="plain">
        {{ problem.oj }}
      </el-tag>
    </div>

    <div v-if="showTags" class="tag-row">
      <el-tag v-for="tag in problem.tags || []" :key="tag" effect="plain">
        {{ tag }}
      </el-tag>
      <span v-if="!problem.tags?.length" class="muted">暂无标签</span>
    </div>

    <div class="statement">
      <div v-if="qojStatementUrl" class="qoj-statement">
        <div>
          <strong>QOJ PDF 题面</strong>
          <span>QOJ 的样例、输入格式和输出格式通常都在 PDF 中。为避免页面自动下载，这里只提供打开入口。</span>
        </div>
        <el-button type="primary" plain size="small" @click="openQojStatement">
          打开题面
        </el-button>
      </div>

      <MarkdownViewer :content="visibleStatementMarkdown" />
    </div>

    <el-descriptions :column="1" border>
      <el-descriptions-item label="输入格式">
        {{ problem.inputDescription || '-' }}
      </el-descriptions-item>

      <el-descriptions-item label="输出格式">
        {{ problem.outputDescription || '-' }}
      </el-descriptions-item>
    </el-descriptions>

    <div class="samples">
      <div class="sample-block">
        <div class="sample-title">样例输入</div>
        <pre>{{ problem.sampleInput || '-' }}</pre>
      </div>

      <div class="sample-block">
        <div class="sample-title">样例输出</div>
        <pre>{{ problem.sampleOutput || '-' }}</pre>
      </div>
    </div>

    <div class="actions">
      <el-button type="primary" plain @click="$emit('toggle-tags')">
        {{ showTags ? '隐藏题目标签' : '展示题目标签' }}
      </el-button>

      <el-button type="success" plain @click="openOriginalProblem">
        跳转原题
      </el-button>

      <span class="submit-note">提交题目请在原题网站提交</span>

      <el-button
        v-if="stage === 'solving'"
        type="warning"
        @click="$emit('finish-thinking')"
      >
        我已完成独立思考
      </el-button>

      <el-button
        v-if="stage === 'reviewing'"
        type="primary"
        plain
        @click="$emit('toggle-debug')"
      >
        {{ showDebugInfo ? '隐藏调度信息' : '查看调度信息' }}
      </el-button>
    </div>
  </section>
</template>

<script setup>
import { computed } from 'vue'
import MarkdownViewer from '../MarkdownViewer.vue'

const props = defineProps({
  problem: {
    type: Object,
    required: true
  },
  stage: {
    type: String,
    required: true
  },
  showDebugInfo: {
    type: Boolean,
    default: false
  },
  showTags: {
    type: Boolean,
    default: false
  }
})

defineEmits(['finish-thinking', 'toggle-debug', 'toggle-tags'])

const statementText = computed(() => props.problem?.statementMarkdown || '暂无题面。')

const qojStatementUrl = computed(() => {
  const match = statementText.value.match(/^ALGOFORGE_(?:QOJ|PDF)_STATEMENT:\s*(\S+)/m)
  return match ? match[1] : ''
})

const visibleStatementMarkdown = computed(() => {
  return statementText.value
    .replace(/^ALGOFORGE_(?:QOJ|PDF)_STATEMENT:\s*\S+\s*/m, '')
    .trim()
})

const openQojStatement = () => {
  const target = qojStatementUrl.value || props.problem?.url
  if (target) {
    window.open(target, '_blank', 'noopener,noreferrer')
  }
}

const openOriginalProblem = () => {
  if (props.problem?.url) {
    window.open(props.problem.url, '_blank', 'noopener,noreferrer')
  }
}
</script>

<style scoped>
.problem-section {
  padding: 24px;
  border-radius: 8px;
  background: #ffffff;
  border: 1px solid #ebeef5;
}

.problem-title-row {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 16px;
}

.problem-title-row h2 {
  margin: 0;
  font-size: 26px;
  line-height: 1.3;
}

.problem-meta {
  margin-top: 6px;
  color: #606266;
}

.tag-row {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 18px;
}

.muted {
  color: #909399;
}

.statement {
  margin: 18px 0 22px;
}

.qoj-statement {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 18px;
  padding: 14px 16px;
  border: 1px solid #dcdfe6;
  border-radius: 8px;
  background: #f8fafc;
}

.qoj-statement div {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.qoj-statement span {
  color: #606266;
  font-size: 13px;
}

.samples {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
  margin-top: 18px;
}

.sample-block {
  border: 1px solid #ebeef5;
  border-radius: 8px;
  overflow: hidden;
}

.sample-title {
  padding: 10px 12px;
  color: #606266;
  background: #f5f7fa;
  border-bottom: 1px solid #ebeef5;
}

pre {
  margin: 0;
  padding: 14px;
  min-height: 72px;
  overflow-x: auto;
  background: #1f2937;
  color: #e5e7eb;
}

.actions {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 12px;
  margin-top: 24px;
}

.submit-note {
  color: #606266;
  font-size: 14px;
}

@media (max-width: 760px) {
  .problem-title-row,
  .qoj-statement {
    flex-direction: column;
  }

  .samples {
    grid-template-columns: 1fr;
  }
}
</style>
