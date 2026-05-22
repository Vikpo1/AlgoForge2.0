# AlgoForge

AlgoForge 是一个面向算法训练者的智能复习平台，目标不是重新实现一个 Online Judge，而是把外部 OJ 题目、个人题单、复习调度、防剧透笔记和掌握度反馈整合成一个闭环训练系统。

项目核心理念是：用户在复习算法题时，既需要完整记录思路、贴士、笔记和题解，又不能在重新思考前被标签或答案剧透；同时，错题和待复习题不应该只停留在静态列表里，而应该根据掌握程度和复习状态动态进入下一次训练。

## 核心功能

### 1. 题目导入

用户可以在主菜单中粘贴外部 OJ 题目链接，并选择加入某个已有题单。

当前导入流程支持：

- Codeforces 题目链接
- AtCoder 题目链接
- 洛谷题目链接
- QOJ 题目链接

导入时后端会尝试抓取真实题面，并写入数据库中的题目信息字段，包括：

- 题目标题
- 来源 OJ
- 原题链接
- 题面 Markdown
- 输入格式
- 输出格式
- 样例输入
- 样例输出
- 题目标签

如果题面爬取失败，导入不会中断。系统会保留原题链接，并写入占位提示，确保用户仍然可以通过“跳转原题”继续使用。

### 2. 多 OJ 题面爬取

后端提供统一的题面爬取入口，根据 URL 自动识别题目平台。

当前实现采用 C++ 后端优先调用 Python 辅助爬虫的方式：

- Codeforces 使用 `cloudscraper` 以提高公开题面抓取成功率。
- AtCoder 解析 `task-statement` 页面结构。
- 洛谷优先解析页面中的 `lentille-context` JSON。
- QOJ 题面通常是 PDF，因此站内不强行解析样例，而是提供原题/PDF 打开入口。

QOJ 的样例、输入格式和输出格式通常都在 PDF 内，因此 AlgoForge 不会自动弹出下载或内嵌触发下载，而是在刷题页提供“打开题面”按钮。

### 3. 题单管理

用户可以在题单管理页维护自己的复习题单。

支持操作：

- 创建题单
- 自定义题单名称
- 自定义题单描述
- 修改题单权值
- 删除题单
- 查看题单内题目
- 修改题单内单题权值

题单权值和单题权值都会影响调度概率。权值修改采用自动保存方式，用户不需要逐项点击保存。

### 4. 智能调度与状态机

AlgoForge 使用“题单权值 + 单题调度权值”的方式进行复习调度。

当前题目复习状态包括 5 类：

- 赛后首补
- 冷却重刷
- 穿插特训
- 已掌握
- 已归档

系统会根据用户反馈更新题目状态：

- 完全不会：进入赛后首补
- 看贴士才会：进入冷却重刷
- 独立做出但较慢：进入冷却重刷
- 秒杀：进入已掌握

系统还会根据反馈设置下一次复习时间：

- 完全不会：立即可再次出现
- 看贴士才会：约 24 小时后再次进入复习池
- 独立做出但较慢：约 72 小时后再次进入复习池
- 秒杀：约 168 小时后再次进入复习池

调度时会过滤仍处于冷却期的题目，避免刚刚标记为“秒杀”的题马上再次频繁出现。

### 5. 防剧透刷题流程

刷题页强调“先独立思考，再展开材料”。

进入题目后，默认只展示：

- 题面
- 输入输出格式
- 样例
- 跳转原题入口

默认隐藏：

- 题目标签
- 贴士
- 完整笔记
- 调度信息

用户完成独立思考后，可以点击进入复盘阶段，再选择是否展开贴士和完整笔记。

### 6. 指定题目与随机刷题

刷题页支持两种模式：

- 指定题目：先选择题单，再选择该题单中的具体题目。
- 随机：从当前可复习题目池中随机抽题。

指定题目模式适合主动复盘某道题；随机模式适合日常训练和抗遗忘复习。

### 7. 贴士与笔记

每道题都可以保存两类个人材料：

- 贴士：轻量提示，适合只给一点方向，不直接剧透完整解法。
- 完整笔记：完整复盘内容，可记录思路、题解、卡点、易错点、代码要点等。

刷题页进入复盘阶段后，用户可以展开贴士或笔记，直接编辑 Markdown 内容并保存到数据库。

前端支持 Markdown 渲染，并对常见数学公式进行显示处理。

### 8. 原题网站提交

AlgoForge 不在站内判题，也不代替用户向 OJ 提交代码。

刷题页保留“跳转原题”按钮，并提示用户：

> 提交题目请在原题网站提交

这样可以避免 Cookie 代交、远程判题接口变化和本地判题环境配置等复杂问题，让 AlgoForge 专注于复习管理与防剧透体验。

## 技术架构

项目采用 B/S 架构。

### 前端

前端位于 `frontend/`，基于：

- Vue 3
- Vite
- Element Plus
- Axios
- markdown-it

主要负责：

- 主菜单与题目导入
- 题单管理
- 刷题与复盘交互
- Markdown 渲染
- 防剧透展示控制
- 贴士和笔记编辑

### 后端

后端位于 `backend/`，基于：

- C++17
- cpp-httplib
- nlohmann/json
- CMake
- MySQL client library

主要负责：

- HTTP API 服务
- 题单和题目管理
- 复习状态机
- 调度权重计算
- MySQL 数据持久化
- 调用 Python 爬虫辅助进程

### 爬虫辅助脚本

Python 爬虫位于 `backend/scripts/problem_crawler.py`。

依赖位于：

```text
backend/scripts/requirements.txt
```

当前依赖：

- `requests`
- `beautifulsoup4`
- `cloudscraper`

## 数据库

项目使用 MySQL 存储核心数据。

主要数据包括：

- 用户
- 题单
- 题目
- 题单与题目的关联关系
- 复习状态
- 贴士与笔记
- 反馈记录
- 历史判题提交记录表

数据库 schema 位于：

```text
backend/data/db_schema.sql
```

初始化数据位于：

```text
backend/data/db_seed.sql
```

数据库连接环境变量示例位于：

```text
backend/.env.example
```

## 本地启动

### 1. 安装前端依赖

```bash
cd frontend
npm install
```

### 2. 启动前端

```bash
cd frontend
npm run dev
```

默认访问：

```text
http://localhost:5173
```

### 3. 安装后端依赖

Ubuntu / WSL 示例：

```bash
sudo apt update
sudo apt install mysql-server default-libmysqlclient-dev pkg-config cmake g++ python3 python3-pip
```

安装 Python 爬虫依赖：

```bash
cd backend
pip install -r scripts/requirements.txt
```

### 4. 初始化 MySQL

```bash
cd backend
export ALGOFORGE_DB_ROOT_USER=root
export ALGOFORGE_DB_ROOT_PASSWORD=你的 root 密码
export ALGOFORGE_DB_USER=algoforge
export ALGOFORGE_DB_PASSWORD=algoforge
export ALGOFORGE_DB_NAME=algoforge

bash scripts/init_mysql.sh
```

如果 MySQL root 没有密码，可以不设置 `ALGOFORGE_DB_ROOT_PASSWORD`。

### 5. 构建后端

```bash
cd backend
cmake -S . -B build -DALGOFORGE_USE_MYSQL=ON
cmake --build build
```

### 6. 启动后端

```bash
cd backend
export ALGOFORGE_DB_HOST=127.0.0.1
export ALGOFORGE_DB_PORT=3306
export ALGOFORGE_DB_USER=algoforge
export ALGOFORGE_DB_PASSWORD=algoforge
export ALGOFORGE_DB_NAME=algoforge

./build/AlgoArchiveServer
```

后端默认监听：

```text
http://localhost:8080
```

健康检查：

```text
http://localhost:8080/api/health
http://localhost:8080/api/db/health
```

## 主要 API

部分核心接口：

- `GET /api/health`
- `GET /api/db/health`
- `GET /api/problem-lists`
- `POST /api/problem-lists`
- `PATCH /api/problem-lists/:id`
- `DELETE /api/problem-lists/:id`
- `GET /api/problem-lists/:id/problems`
- `POST /api/problems/import`
- `GET /api/problems/:id/detail`
- `PATCH /api/problems/:id/weight`
- `GET /api/problems/:id/review-material`
- `PATCH /api/problems/:id/review-material`
- `GET /api/review/next`
- `POST /api/review/:id/feedback`

## 当前限制

- QOJ 题面通常是 PDF，当前不解析 PDF 内样例，只提供原题打开入口。
- Codeforces 可能受到 Cloudflare 或网络环境影响，爬取失败时会保留原题链接和占位题面。
- 当前项目默认是单用户模型，后端使用固定用户 ID。
- 站内不提供代码提交和在线判题，提交统一回到原题网站完成。
- Python 爬虫依赖本机 Python 环境，部署时需要安装 `backend/scripts/requirements.txt`。

## 项目定位

AlgoForge 不是一个“大而全”的刷题平台，而是一个算法复盘效率工具。

它关注的是：

- 如何管理做过、没做会、需要补的题
- 如何减少“今天复习哪道题”的选择成本
- 如何避免笔记和标签提前剧透
- 如何把题目状态、调度权重和个人复盘材料连接成闭环

一句话概括：

> AlgoForge 是一个通过“题单权重调度 + 复习状态机 + 防剧透笔记”来重构算法题复习流程的 Web 平台。
