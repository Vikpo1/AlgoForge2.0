<template>
  <el-card class="material-card" shadow="never">
    <template #header>
      <div class="material-header">
        <span>复盘材料</span>
      </div>
    </template>

    <el-skeleton v-if="loading" :rows="4" animated />

    <el-alert
      v-if="errorMessage"
      type="error"
      show-icon
      :title="errorMessage"
    />

    <div v-if="material && !loading" class="material-content">
      <div class="material-actions">
        <el-button type="warning" plain @click="$emit('toggle-hint')">
          {{ showHint ? '隐藏贴士' : '展开贴士' }}
        </el-button>

        <el-button type="primary" plain @click="$emit('toggle-note')">
          {{ showNote ? '隐藏笔记' : '展开笔记' }}
        </el-button>
      </div>

      <el-card v-if="showHint" class="material-editor" shadow="never">
        <div class="editor-header">
          <h3>贴士</h3>
          <el-button
            type="primary"
            size="small"
            :loading="saving"
            @click="saveMaterial"
          >
            保存
          </el-button>
        </div>

        <el-input
          v-model="draft.hintMarkdown"
          type="textarea"
          :rows="5"
          placeholder="写一点轻提示，复习时先给自己一个不剧透的方向。"
        />

        <div class="preview-title">预览</div>
        <MarkdownViewer :content="draft.hintMarkdown || '暂无贴士。'" />
      </el-card>

      <el-card v-if="showNote" class="material-editor" shadow="never">
        <div class="editor-header">
          <h3>完整笔记</h3>
          <el-button
            type="primary"
            size="small"
            :loading="saving"
            @click="saveMaterial"
          >
            保存
          </el-button>
        </div>

        <el-input
          v-model="draft.noteMarkdown"
          type="textarea"
          :rows="9"
          placeholder="记录完整题解、卡点、易错点和复盘总结。支持 Markdown。"
        />

        <div class="preview-title">预览</div>
        <MarkdownViewer :content="draft.noteMarkdown || '暂无笔记。'" />
      </el-card>
    </div>
  </el-card>
</template>

<script setup>
import { reactive, watch } from 'vue'
import MarkdownViewer from '../MarkdownViewer.vue'

const props = defineProps({
  loading: {
    type: Boolean,
    default: false
  },
  errorMessage: {
    type: String,
    default: ''
  },
  material: {
    type: Object,
    default: null
  },
  showHint: {
    type: Boolean,
    default: false
  },
  showNote: {
    type: Boolean,
    default: false
  },
  saving: {
    type: Boolean,
    default: false
  }
})

const emit = defineEmits(['toggle-hint', 'toggle-note', 'save-material'])

const draft = reactive({
  hintMarkdown: '',
  noteMarkdown: ''
})

watch(
  () => props.material,
  (material) => {
    draft.hintMarkdown = material?.hintMarkdown || ''
    draft.noteMarkdown = material?.noteMarkdown || ''
  },
  { immediate: true }
)

const saveMaterial = () => {
  emit('save-material', {
    hintMarkdown: draft.hintMarkdown,
    noteMarkdown: draft.noteMarkdown
  })
}
</script>

<style scoped>
.material-card {
  margin-top: 16px;
}

.material-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.material-content {
  margin-top: 4px;
}

.material-actions {
  display: flex;
  gap: 12px;
  margin-bottom: 20px;
}

.material-editor {
  margin-top: 16px;
}

.editor-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 12px;
}

.editor-header h3 {
  margin: 0;
}

.preview-title {
  margin: 18px 0 8px;
  color: #606266;
  font-size: 13px;
}
</style>
