# Scripts 目录说明

## 📁 脚本分类

### 🐍 Python 环境管理
- **setup_python_env.sh** - 设置 Python 虚拟环境（安装 uv、创建 .venv、安装依赖）
- **activate_env.sh** - 激活 Python 虚拟环境

### 🤗 Hugging Face Content 管理
- **setup_hf_content.sh** - 下载/更新 Content 资源（支持多种操作：status, log, rollback 等）
- **upload_content_to_hf.sh** - 上传 Content 到 Hugging Face（维护者使用）

### 🤖 UE5 通信脚本（Python）
- **send_skill_list.py** - 发送技能列表到 UE5 并接收反馈
- **mock_backend.py** - 模拟后端服务器，用于测试
- **tcp_stream_viewer.py** - TCP 流查看器，用于调试网络通信

### 🗺️ 场景图处理（Python）
- **scene_graph_postprocessor.py** - 场景图后处理
- **fix_scene_graph_labels.py** - 修复场景图标签
- **merge_scene_graph_nodes.py** - 合并场景图节点

### 🔧 开发工具（Python）
- **refactor_core.py** - 代码重构工具

### 📂 数据集工具
- **dataset/** - 数据集相关脚本目录

## 🚀 快速开始

### 新用户首次设置

```bash
# 一键完成所有设置
./scripts/setup.sh
```

这会自动：
1. 检查并设置 Python 环境
2. 检查并下载 Content 资源
3. 验证环境配置

### 日常使用

```bash
# 更新 Content 资源
./scripts/setup.sh --update-content

# 更新 Python 依赖
./scripts/setup.sh --update-python

# 查看状态
./scripts/setup.sh --status
```

## 📝 详细说明

### Python 环境管理

#### setup_python_env.sh
创建和管理 Python 虚拟环境。

**功能：**
- 自动安装 uv（如果未安装）
- 创建 `.venv` 虚拟环境
- 安装 requirements.txt 中的依赖
- 支持重新创建环境

**使用：**
```bash
./scripts/setup_python_env.sh
```

#### activate_env.sh
快速激活虚拟环境的辅助脚本。

**使用：**
```bash
source scripts/activate_env.sh
```

### Hugging Face Content 管理

#### setup_hf_content.sh
管理 UE5 Content 资源的主要脚本。

**功能：**
- 首次克隆 Content 仓库
- 更新到最新版本
- 查看仓库状态
- 查看提交历史
- 版本回滚
- 强制更新
- 清理重建

**使用：**
```bash
# 克隆或更新
./scripts/setup_hf_content.sh

# 查看状态
./scripts/setup_hf_content.sh --status

# 查看历史
./scripts/setup_hf_content.sh --log 20

# 强制更新
./scripts/setup_hf_content.sh --force

# 帮助
./scripts/setup_hf_content.sh --help
```

#### upload_content_to_hf.sh
上传 Content 到 Hugging Face（仅维护者使用）。

**功能：**
- 初始化 Content 为 Git 仓库
- 配置 Git LFS
- 提交并推送更改

**使用：**
```bash
./scripts/upload_content_to_hf.sh
```

### Python 脚本

所有 Python 脚本需要先激活虚拟环境：

```bash
source .venv/bin/activate
python scripts/send_skill_list.py
```

## 🔄 工作流程

### 开发者工作流

1. **首次设置**
   ```bash
   ./scripts/setup.sh
   ```

2. **日常开发**
   ```bash
   # 激活环境
   source .venv/bin/activate
   
   # 运行脚本
   python scripts/send_skill_list.py
   ```

3. **更新资源**
   ```bash
   ./scripts/setup.sh --update-content
   ```

### 维护者工作流

1. **更新 Content**
   ```bash
   cd unreal_project/Content
   # 修改文件...
   git add .
   git commit -m "Update content"
   git push
   ```

2. **或使用脚本**
   ```bash
   ./scripts/upload_content_to_hf.sh
   ```

## 💡 最佳实践

1. **使用统一脚本** - 优先使用 `setup.sh` 进行自动化设置
2. **定期更新** - 定期运行 `./scripts/setup.sh --update-content` 获取最新资源
3. **虚拟环境** - 始终在虚拟环境中运行 Python 脚本
4. **查看帮助** - 不确定时使用 `--help` 查看帮助信息
