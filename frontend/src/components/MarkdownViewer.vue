<template>
  <div ref="viewerRef" class="markdown-viewer" v-html="renderedHtml"></div>
</template>

<script setup>
import { computed, nextTick, onMounted, ref, watch } from 'vue'
import MarkdownIt from 'markdown-it'

const props = defineProps({
  content: {
    type: String,
    default: ''
  }
})

const viewerRef = ref(null)

const md = new MarkdownIt({
  html: false,
  linkify: true,
  breaks: true
})

const mathPlaceholders = []

const escapeHtml = (value) => {
  return String(value)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;')
}

const protectMath = (source) => {
  mathPlaceholders.length = 0

  const pushMath = (tex, display) => {
    const token = `ALGOMATH${mathPlaceholders.length}TOKEN`
    const escaped = escapeHtml(tex.trim())
    mathPlaceholders.push(
      display
        ? `<div class="math-block">\\[${escaped}\\]</div>`
        : `<span class="math-inline">\\(${escaped}\\)</span>`
    )
    return token
  }

  return (source || '')
    .replace(/\$\$([\s\S]+?)\$\$/g, (_, tex) => pushMath(tex, true))
    .replace(/(^|[^\\])\$([^\n$]+?)\$/g, (_, prefix, tex) => `${prefix}${pushMath(tex, false)}`)
}

const restoreMath = (html) => {
  return mathPlaceholders.reduce((output, replacement, index) => {
    return output.replaceAll(`ALGOMATH${index}TOKEN`, replacement)
  }, html)
}

const renderedHtml = computed(() => {
  const protectedSource = protectMath(props.content || '')
  return restoreMath(md.render(protectedSource))
})

const ensureMathJax = () => {
  if (window.MathJax?.typesetPromise) {
    return Promise.resolve()
  }

  if (window.__algoforgeMathJaxPromise) {
    return window.__algoforgeMathJaxPromise
  }

  if (!window.__algoforgeMathJaxPromise) {
    window.MathJax = {
      tex: {
        inlineMath: [['\\(', '\\)']],
        displayMath: [['\\[', '\\]']]
      },
      options: {
        skipHtmlTags: ['script', 'noscript', 'style', 'textarea', 'pre', 'code']
      }
    }

    window.__algoforgeMathJaxPromise = new Promise((resolve) => {
      const script = document.createElement('script')
      script.src = 'https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-chtml.js'
      script.async = true
      script.onload = resolve
      script.onerror = resolve
      document.head.appendChild(script)
    })
  }

  return window.__algoforgeMathJaxPromise
}

const typesetMath = async () => {
  await nextTick()
  await ensureMathJax()
  await nextTick()

  if (window.MathJax?.typesetPromise && viewerRef.value) {
    if (window.MathJax.typesetClear) {
      window.MathJax.typesetClear([viewerRef.value])
    }
    window.MathJax.typesetPromise([viewerRef.value]).catch(() => {})
  }
}

watch(renderedHtml, typesetMath, { immediate: true })
onMounted(typesetMath)
</script>

<style scoped>
.markdown-viewer {
  line-height: 1.8;
  color: #303133;
}

.markdown-viewer :deep(h1),
.markdown-viewer :deep(h2),
.markdown-viewer :deep(h3) {
  margin: 16px 0 12px;
  font-weight: 700;
}

.markdown-viewer :deep(p) {
  margin: 10px 0;
}

.markdown-viewer :deep(ul),
.markdown-viewer :deep(ol) {
  padding-left: 24px;
  margin: 12px 0;
}

.markdown-viewer :deep(li) {
  margin: 6px 0;
}

.markdown-viewer :deep(code) {
  padding: 2px 6px;
  border-radius: 4px;
  background: #f2f3f5;
  font-family: Consolas, Monaco, monospace;
}

.markdown-viewer :deep(pre) {
  padding: 14px;
  border-radius: 8px;
  overflow-x: auto;
  background: #1f2937;
  color: #e5e7eb;
}

.markdown-viewer :deep(pre code) {
  padding: 0;
  background: transparent;
  color: inherit;
}

.markdown-viewer :deep(.math-inline) {
  display: inline-block;
  min-width: 0.25em;
}

.markdown-viewer :deep(.math-block) {
  margin: 14px 0;
  overflow-x: auto;
}
</style>
