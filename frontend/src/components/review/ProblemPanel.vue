<template>
  <section class="problem-section" :class="{ 'contest-problem': isContestStyledProblem }">
    <div class="problem-title-row">
      <div>
        <h2>{{ problem.title }}</h2>
        <div v-if="!isContestStyledProblem" class="problem-meta">
          {{ problem.oj }} · {{ problem.difficulty }}
        </div>
        <div v-else-if="contestLimits.length" class="contest-limits">
          <div v-for="limit in contestLimits" :key="limit">{{ limit }}</div>
        </div>
      </div>

      <el-tag v-if="!isContestStyledProblem" type="info" effect="plain">
        {{ problem.oj }}
      </el-tag>
    </div>

    <div v-if="showTags" class="tag-row">
      <el-tag v-for="tag in problem.tags || []" :key="tag" effect="plain">
        {{ tag }}
      </el-tag>
      <span v-if="!problem.tags?.length" class="muted">暂无标签</span>
    </div>

    <div class="statement" :class="{ 'statement-contest': isContestStyledProblem }">
      <div v-if="qojStatementUrl" class="qoj-statement">
        <div>
          <strong>QOJ PDF 题面</strong>
          <span>
            QOJ 的样例、输入格式和输出格式通常都在 PDF 中。为避免页面自动下载，这里只提供打开入口。
          </span>
        </div>
        <el-button type="primary" plain size="small" @click="openQojStatement">
          打开题面
        </el-button>
      </div>

      <MarkdownViewer :content="visibleStatementMarkdown" />
    </div>

    <template v-if="!isContestStyledProblem">
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
    </template>

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

const isContestStyledProblem = computed(() => ['Codeforces', 'AtCoder'].includes(props.problem?.oj))
const statementText = computed(() => props.problem?.statementMarkdown || '暂无题面。')

const qojStatementUrl = computed(() => {
  const match = statementText.value.match(/^ALGOFORGE_(?:QOJ|PDF)_STATEMENT:\s*(\S+)/m)
  return match ? match[1] : ''
})

const contestLimits = computed(() => {
  const match = statementText.value.match(
    /^ALGOFORGE_(?:CF|ATCODER)_LIMITS\s*\n([\s\S]*?)\nALGOFORGE_(?:CF|ATCODER)_BODY/m
  )
  if (!match) {
    return []
  }
  return match[1]
    .split('\n')
    .map(line => line.trim())
    .filter(Boolean)
})

const visibleStatementMarkdown = computed(() => {
  let markdown = statementText.value
    .replace(/^ALGOFORGE_(?:QOJ|PDF)_STATEMENT:\s*\S+\s*/m, '')
    .replace(/^ALGOFORGE_(?:CF|ATCODER)_LIMITS\s*\n[\s\S]*?\nALGOFORGE_(?:CF|ATCODER)_BODY\s*/m, '')
    .trim()

  if (isContestStyledProblem.value) {
    markdown = markdown.replace(/^##\s*题面\s*/m, '').trim()
  }

  return markdown
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

.contest-problem {
  max-width: 1040px;
  margin: 0 auto;
  padding: 30px 42px;
  border-color: #d7d7d7;
}

.contest-problem .problem-title-row {
  justify-content: center;
  text-align: center;
  margin-bottom: 36px;
}

.contest-problem .problem-title-row h2 {
  font-size: 26px;
  font-weight: 500;
}

.contest-limits {
  margin-top: 8px;
  color: #000000;
  font-size: 15px;
  line-height: 1.6;
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

.statement-contest {
  max-width: 960px;
  margin: 0 auto 24px;
}

.statement-contest :deep(.markdown-viewer) {
  color: #000000;
  font-size: 16px;
  line-height: 1.55;
}

.statement-contest :deep(.markdown-viewer p) {
  margin: 0 0 18px;
}

.statement-contest :deep(.markdown-viewer h2) {
  margin: 28px 0 12px;
  color: #000000;
  font-size: 20px;
  font-weight: 700;
  line-height: 1.3;
}

.statement-contest :deep(.markdown-viewer h3) {
  margin: 14px 0 8px;
  color: #000000;
  font-size: 16px;
  font-weight: 700;
}

.statement-contest :deep(.markdown-viewer ul),
.statement-contest :deep(.markdown-viewer ol) {
  margin: 12px 0 16px;
  padding-left: 28px;
}

.statement-contest :deep(.markdown-viewer li) {
  margin: 4px 0;
}

.statement-contest :deep(.markdown-viewer pre) {
  margin: 8px 0 16px;
  padding: 12px 14px;
  border: 1px solid #cfcfcf;
  border-radius: 0;
  background: #f7f7f7;
  color: #000000;
  font-family: 'Courier New', Courier, monospace;
  font-size: 15px;
  line-height: 1.4;
}

.statement-contest :deep(.markdown-viewer code) {
  padding: 0;
  background: transparent;
  color: inherit;
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

  .contest-problem {
    padding: 24px 18px;
  }

  .samples {
    grid-template-columns: 1fr;
  }
}
</style>
