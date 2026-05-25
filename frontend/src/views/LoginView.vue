<template>
  <main class="login-page">
    <section class="login-panel">
      <div class="hero">
        <h1>AlgoForge</h1>
        <p>登录后查看你的题单、题目记录和刷题统计。</p>
      </div>

      <el-card shadow="never" class="form-card">
        <el-tabs v-model="mode">
          <el-tab-pane label="登录" name="login" />
          <el-tab-pane label="注册" name="register" />
        </el-tabs>

        <el-form label-position="top">
          <el-form-item label="用户名">
            <el-input v-model="form.username" placeholder="请输入用户名" />
          </el-form-item>

          <el-form-item v-if="mode === 'register'" label="邮箱">
            <el-input v-model="form.email" placeholder="可选" />
          </el-form-item>

          <el-form-item label="密码">
            <el-input v-model="form.password" type="password" show-password placeholder="请输入密码" />
          </el-form-item>

          <el-alert
            v-if="errorMessage"
            class="message"
            type="error"
            :title="errorMessage"
            show-icon
          />

          <el-button type="primary" :loading="submitting" class="submit" @click="submit">
            {{ mode === 'login' ? '登录' : '注册并登录' }}
          </el-button>

          <el-alert
            class="hint"
            type="info"
            show-icon
            title="旧数据兼容账号：local_user / algoforge"
          />
        </el-form>
      </el-card>
    </section>
  </main>
</template>

<script setup>
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { login, register } from '../api/authApi'
import { setAuthSession } from '../utils/auth'

const router = useRouter()
const mode = ref('login')
const submitting = ref(false)
const errorMessage = ref('')

const form = reactive({
  username: '',
  email: '',
  password: ''
})

const submit = async () => {
  errorMessage.value = ''
  if (!form.username.trim() || !form.password.trim()) {
    errorMessage.value = '请输入用户名和密码'
    return
  }

  submitting.value = true
  try {
    const request = mode.value === 'login'
      ? login({ username: form.username.trim(), password: form.password })
      : register({
          username: form.username.trim(),
          password: form.password,
          email: form.email.trim()
        })

    const response = await request
    if (response.data.code === 0) {
      setAuthSession(response.data.data)
      ElMessage.success(mode.value === 'login' ? '登录成功' : '注册成功')
      router.push('/')
    } else {
      errorMessage.value = response.data.message || '操作失败'
    }
  } catch (error) {
    errorMessage.value = error.response?.data?.message || '登录失败，请检查后端和数据库状态'
  } finally {
    submitting.value = false
  }
}
</script>

<style scoped>
.login-page {
  min-height: 100vh;
  display: grid;
  place-items: center;
  padding: 32px;
  box-sizing: border-box;
}

.login-panel {
  width: min(960px, 100%);
  display: grid;
  grid-template-columns: minmax(0, 1.1fr) minmax(320px, 420px);
  gap: 28px;
  align-items: stretch;
}

.hero {
  padding: 36px;
  background: #111827;
  color: #f9fafb;
  border-radius: 8px;
}

.hero h1 {
  margin: 0;
  font-size: 42px;
}

.hero p {
  margin: 14px 0 0;
  max-width: 28rem;
  color: #cbd5e1;
  line-height: 1.7;
}

.form-card {
  border-radius: 8px;
}

.submit {
  width: 100%;
  margin-top: 8px;
}

.message,
.hint {
  margin-top: 12px;
}

@media (max-width: 860px) {
  .login-panel {
    grid-template-columns: 1fr;
  }
}
</style>
