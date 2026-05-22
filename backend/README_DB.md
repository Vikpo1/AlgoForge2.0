# AlgoForge MySQL 接入

当前后端默认仍使用内存仓库，保证页面可以直接运行。要测试 MySQL 连接，需要重新编译时打开 `ALGOFORGE_USE_MYSQL`。

## 1. 安装依赖

Ubuntu / WSL:

```bash
sudo apt update
sudo apt install mysql-server default-libmysqlclient-dev pkg-config
```

## 2. 初始化数据库

在 `backend` 目录下执行：

```bash
export ALGOFORGE_DB_ROOT_USER=root
export ALGOFORGE_DB_ROOT_PASSWORD=你的root密码
export ALGOFORGE_DB_USER=algoforge
export ALGOFORGE_DB_PASSWORD=algoforge
export ALGOFORGE_DB_NAME=algoforge

bash scripts/init_mysql.sh
```

如果 root 没有密码，可以不设置 `ALGOFORGE_DB_ROOT_PASSWORD`。

## 3. 开启 MySQL 支持编译

```bash
cmake -S . -B build -DALGOFORGE_USE_MYSQL=ON
cmake --build build
```

## 4. 启动后端

```bash
export ALGOFORGE_DB_HOST=127.0.0.1
export ALGOFORGE_DB_PORT=3306
export ALGOFORGE_DB_USER=algoforge
export ALGOFORGE_DB_PASSWORD=algoforge
export ALGOFORGE_DB_NAME=algoforge

./build/AlgoArchiveServer
```

## 5. 验证连接

浏览器打开：

```text
http://localhost:8080/api/db/health
```

如果返回：

```json
{
  "connected": true,
  "enabled": true
}
```

说明 C++ 后端已经真正连上 MySQL。
