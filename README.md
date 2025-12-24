# MultiAgent-Unreal

基于 Unreal Engine 5 的多智能体仿真框架，用于机器人、无人机、角色等异构智能体的协同仿真。

## 资产下载

项目资产托管在 Hugging Face，克隆代码后需要下载 Content 资产：

```bash
# 安装 Hugging Face CLI (如果没有)
curl -LsSf https://hf.co/cli/install.sh | bash

# 下载 Content.zip
hf download MiangChen/MultiAgent-Unreal Content.zip --repo-type=dataset --local-dir=./unreal_project

# 解压到 Content 文件夹
cd unreal_project
unzip Content.zip
```

**要求：**
- Unreal Engine 5.5 (源码编译版本)
- 需要先编译 C++ 代码再打开编辑器

**启用的插件：** GameplayAbilities, StateTree, MassEntity, MassGameplay, MassAI

## 快速开始

### 编译

```bash
# 默认编译 (Development)
./compile.sh

# Debug 配置
./compile.sh -c Debug

# 完全重新编译
./compile.sh -r

# 编译发布版游戏 (不含编辑器)
./compile.sh -g -c Shipping

# 查看帮助
./compile.sh -h
```

### 运行

```
./start.sh
```



## 文档

详细文档请查看 [doc/README.md](doc/README.md)

| 文档 | 说明 |
|------|------|
| [doc/ARCHITECTURE.md](doc/ARCHITECTURE.md) | 系统架构、模块设计 |
| [doc/KEYBINDINGS.md](doc/KEYBINDINGS.md) | 按键说明 |
| [doc/guide/NEW_FEATURE_GUIDE.md](doc/guide/NEW_FEATURE_GUIDE.md) | 新功能开发指南 |

## 项目结构

```
MultiAgent-Unreal/
├── config/                 # Agent 配置 (JSON)
├── unreal_project/         # UE5 项目
│   └── Source/MultiAgent/  # C++ 源码
├── doc/                    # 文档
├── start.sh                # 启动脚本
└── compile.sh              # 编译脚本
```
