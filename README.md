# MultiAgent-Unreal

基于 Unreal Engine 5 的多智能体仿真框架，用于机器人、无人机、角色等异构智能体的协同仿真。

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

使用 UE5 编辑器打开 `unreal_project/MultiAgent.uproject`

## 文档

| 文档 | 说明 |
|------|------|
| [ARCHITECTURE.md](doc/ARCHITECTURE.md) | 系统架构、模块设计、API 参考 |
| [KEYBINDINGS.md](doc/KEYBINDINGS.md) | 按键说明 |
| [UE5_API_CHANGES.md](doc/UE5_API_CHANGES.md) | UE5 API 变更说明 |
| [prompt_code_style.md](doc/prompt_code_style.md) | 编码规范 |

## 项目结构

```
MultiAgent-Unreal/
├── config/                 # Agent 配置 (JSON)
├── unreal_project/         # UE5 项目
│   └── Source/MultiAgent/  # C++ 源码
├── doc/                    # 文档
└── compile.sh              # 编译脚本
```
