# Python 环境管理指南

本项目使用 [uv](https://github.com/astral-sh/uv) 管理 Python 环境和依赖。uv 是一个极快的 Python 包管理器，比 pip 快 10-100 倍。

## 快速开始

### 一键设置

```bash
./scripts/setup_python_env.sh
```

这会自动：
1. 安装 uv（如果未安装）
2. 创建虚拟环境（`.venv`）
3. 安装所有依赖

### 激活环境

```bash
# 方式 1: 使用快捷脚本
source scripts/activate_env.sh

# 方式 2: 直接激活
source .venv/bin/activate
```

### 退出环境

```bash
deactivate
```

## 手动设置

### 1. 安装 uv

```bash
# Linux/macOS
curl -LsSf https://astral.sh/uv/install.sh | sh

# 或使用 pip
pip install uv
```

### 2. 创建虚拟环境

```bash
uv venv
```

这会在项目根目录创建 `.venv` 目录。

### 3. 安装依赖

```bash
# 激活环境
source .venv/bin/activate

# 安装依赖
uv pip install -r requirements.txt
```

## 依赖管理

### 查看已安装的包

```bash
uv pip list
```

### 添加新依赖

```bash
# 安装包
uv pip install <package-name>

# 更新 requirements.txt
uv pip freeze > requirements.txt
```

### 更新依赖

```bash
# 更新所有包
uv pip install -r requirements.txt --upgrade

# 更新特定包
uv pip install --upgrade <package-name>
```

### 卸载包

```bash
uv pip uninstall <package-name>
```

## 项目依赖

### 核心依赖

- **numpy**: 数值计算（用于 `tcp_stream_viewer.py`）
- **huggingface-hub**: Hugging Face 集成（用于 Content 管理）

### 开发依赖（可选）

```bash
# 安装开发工具
uv pip install pytest black ruff
```

- **pytest**: 测试框架
- **black**: 代码格式化
- **ruff**: 快速 linter

## Python 脚本

项目中的 Python 脚本位于 `scripts/` 目录：

- `send_skill_list.py` - 发送技能列表并接收反馈
- `mock_backend.py` - 模拟后端服务器
- `tcp_stream_viewer.py` - TCP 流查看器
- `scene_graph_postprocessor.py` - 场景图后处理
- `fix_scene_graph_labels.py` - 修复场景图标签
- `merge_scene_graph_nodes.py` - 合并场景图节点

## 使用示例

### 运行脚本

```bash
# 激活环境
source .venv/bin/activate

# 运行脚本
python scripts/send_skill_list.py --help
python scripts/mock_backend.py
```

### 在脚本中使用

所有 Python 脚本都可以直接运行，前提是已激活虚拟环境：

```bash
source .venv/bin/activate
python scripts/send_skill_list.py
```

## 常见问题

### Q: uv 和 pip 有什么区别？

uv 是用 Rust 编写的，比 pip 快得多：
- 依赖解析速度快 10-100 倍
- 安装速度快 10-100 倍
- 完全兼容 pip 的命令和 requirements.txt

### Q: 可以使用 pip 吗？

可以，但推荐使用 uv。如果必须使用 pip：

```bash
source .venv/bin/activate
pip install -r requirements.txt
```

### Q: 虚拟环境在哪里？

虚拟环境位于项目根目录的 `.venv/` 文件夹中。这个目录已添加到 `.gitignore`，不会被提交到 Git。

### Q: 如何重置环境？

```bash
# 删除虚拟环境
rm -rf .venv

# 重新创建
./scripts/setup_python_env.sh
```

### Q: Python 版本要求？

项目要求 Python 3.10 或更高版本。推荐使用 Python 3.12。

检查版本：
```bash
python --version
```

## 最佳实践

1. **始终在虚拟环境中工作**
   ```bash
   source .venv/bin/activate
   ```

2. **更新依赖后同步 requirements.txt**
   ```bash
   uv pip freeze > requirements.txt
   ```

3. **定期更新依赖**
   ```bash
   uv pip install -r requirements.txt --upgrade
   ```

4. **使用 .python-version 文件**
   项目根目录的 `.python-version` 文件指定了推荐的 Python 版本。

## 相关链接

- [uv 官方文档](https://github.com/astral-sh/uv)
- [Python 虚拟环境指南](https://docs.python.org/3/tutorial/venv.html)
- [requirements.txt 格式](https://pip.pypa.io/en/stable/reference/requirements-file-format/)
