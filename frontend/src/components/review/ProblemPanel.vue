<template>
  <section class="problem-section" :class="{ 'contest-problem': isContestStyledProblem, 'qoj-problem': isQojProblem }">
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
      <el-tag v-for="tag in displayTags" :key="tag" effect="plain">
        {{ tag }}
      </el-tag>
      <span v-if="!displayTags.length" class="muted">暂无标签</span>
    </div>

    <div class="statement" :class="{ 'statement-contest': isContestStyledProblem }">
      <div v-if="isQojProblem" class="qoj-statement">
        <div class="qoj-statement-content">
          <strong>QOJ PDF 题面</strong>
          <span v-if="qojPdfUrl">
            样例、输入格式和输出格式通常都在 PDF 中。默认不加载，点击后在当前页面预览。
          </span>
          <span v-else>
            当前未识别到可直接预览的 PDF，请打开原题页面查看题面。
          </span>
        </div>
        <div class="qoj-actions">
          <el-button v-if="qojPdfUrl" type="primary" plain size="small" @click="showQojPdf = !showQojPdf">
            {{ showQojPdf ? '隐藏题面预览' : '预览 PDF 题面' }}
          </el-button>
          <el-button plain size="small" @click="openQojStatement">
            打开原题
          </el-button>
        </div>
      </div>

      <div v-if="isQojProblem && qojPdfUrl && showQojPdf" class="qoj-pdf-frame">
        <iframe
          :src="qojPdfPreviewUrl"
          title="QOJ PDF Statement"
        ></iframe>
        <p>
          若浏览器仍显示工具栏或拦截预览，请点击“打开原题”到 QOJ 查看并提交。
        </p>
      </div>

      <div v-else-if="isQojProblem && !qojPdfUrl" class="qoj-no-preview">
        QOJ 返回了站点校验或原题页面，AlgoForge 不会把整个 QOJ 页面嵌入当前页。请点击“打开原题”查看题面。
      </div>

      <MarkdownViewer v-if="shouldRenderStatementMarkdown" :content="visibleStatementMarkdown" />
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
import { computed, ref, watch } from 'vue'
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

const showQojPdf = ref(false)

const isQojProblem = computed(() => props.problem?.oj === 'QOJ')
const isContestStyledProblem = computed(() => ['Codeforces', 'AtCoder', 'QOJ'].includes(props.problem?.oj))
const statementText = computed(() => props.problem?.statementMarkdown || '暂无题面。')
const displayTags = computed(() => {
  const oj = String(props.problem?.oj || '').trim()
  const ojKey = oj.toLowerCase()
  const seen = new Set()
  const realTags = (props.problem?.tags || [])
    .map(tag => String(tag || '').trim())
    .filter(Boolean)
    .filter(tag => tag.toLowerCase() !== ojKey)
    .filter((tag) => {
      const key = tag.toLowerCase()
      if (seen.has(key)) {
        return false
      }
      seen.add(key)
      return true
    })
  return realTags.length ? realTags : (oj ? [oj] : [])
})

const markerValue = (patterns) => {
  for (const pattern of patterns) {
    const match = statementText.value.match(pattern)
    if (match?.[1]) {
      return match[1]
    }
  }
  return ''
}

const isPdfLikeUrl = (url) => /(?:\.pdf(?:[?#]|$)|download\.php)/i.test(url || '')

const qojOriginalUrl = computed(() => {
  const marked = markerValue([
    /^ALGOFORGE_QOJ_STATEMENT:\s*(\S+)/m,
    /^ALGOFORGE_QOJ_ORIGIN:\s*(\S+)/m
  ])
  if (marked && !isPdfLikeUrl(marked)) {
    return marked
  }
  return props.problem?.url || marked || ''
})

const qojPdfUrl = computed(() => {
  const direct = markerValue([
    /^ALGOFORGE_QOJ_PDF:\s*(\S+)/m,
    /^ALGOFORGE_QOJ_PDF_URL:\s*(\S+)/m
  ])
  if (direct) {
    return direct
  }

  const legacy = markerValue([
    /^ALGOFORGE_(?:QOJ|PDF)_STATEMENT:\s*(\S+)/m
  ])
  return isPdfLikeUrl(legacy) ? legacy : ''
})

const withPdfViewerParams = (url) => {
  if (!url) {
    return ''
  }
  const params = 'toolbar=0&navpanes=0&scrollbar=0&page=1&view=FitH'
  const [base, hash = ''] = url.split('#')
  return `${base}#${hash ? `${hash}&${params}` : params}`
}

const qojPdfPreviewUrl = computed(() => withPdfViewerParams(qojPdfUrl.value))

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
    .replace(/^ALGOFORGE_(?:QOJ|PDF)_STATEMENT:\s*\S+\s*/gm, '')
    .replace(/^ALGOFORGE_QOJ_(?:PDF|PDF_URL|ORIGIN):\s*\S+\s*/gm, '')
    .replace(/^ALGOFORGE_(?:CF|ATCODER)_LIMITS\s*\n[\s\S]*?\nALGOFORGE_(?:CF|ATCODER)_BODY\s*/m, '')
    .trim()

  if (isContestStyledProblem.value) {
    markdown = markdown.replace(/^##\s*题面\s*/m, '').trim()
  }

  return markdown
})

const shouldRenderStatementMarkdown = computed(() => {
  if (!visibleStatementMarkdown.value) {
    return false
  }
  return !isQojProblem.value
})

const openQojStatement = () => {
  const target = qojOriginalUrl.value || props.problem?.url
  if (target) {
    window.open(target, '_blank', 'noopener,noreferrer')
  }
}

const openOriginalProblem = () => {
  if (props.problem?.url) {
    window.open(props.problem.url, '_blank', 'noopener,noreferrer')
  }
}

watch(
  () => props.problem?.id,
  () => {
    showQojPdf.value = false
  }
)
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
  padding: 10px 0 16px;
  border-bottom: 1px solid #e5e7eb;
  border-radius: 0;
  background: transparent;
}

.qoj-statement-content {
  display: flex;
  flex-direction: column;
  gap: 4px;
  min-width: 0;
}

.qoj-actions {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 8px;
}

.qoj-statement span {
  color: #606266;
  font-size: 13px;
}

.qoj-pdf-frame {
  margin: 0 0 24px;
  overflow: hidden;
  border: 1px solid #d9d9d9;
  border-radius: 0;
  background: #ffffff;
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.04);
}

.qoj-pdf-frame iframe {
  display: block;
  width: 100%;
  height: min(78vh, 860px);
  border: 0;
  background: #f5f7fa;
}

.qoj-pdf-frame p {
  margin: 0;
  padding: 10px 12px;
  color: #606266;
  font-size: 13px;
  border-top: 1px solid #ebeef5;
  background: #f8fafc;
}

.qoj-no-preview {
  margin: 0 0 18px;
  padding: 12px 14px;
  color: #606266;
  font-size: 13px;
  background: #f8fafc;
  border: 1px solid #e5e7eb;
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
