import { createApp } from 'vue'
import App from './App.vue'
import router from './router'

// 1. 引入 Element Plus 组件库和它的样式文件
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'

const app = createApp(App)

app.use(router)

// 2. 告诉 Vue 使用 Element Plus
app.use(ElementPlus)

app.mount('#app')