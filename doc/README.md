# MultiAgent-Unreal 文档索引

## 🚀 快速开始

| 我想... | 看这个文档 |
|---------|-----------|
| 了解项目整体架构 | [ARCHITECTURE.md](ARCHITECTURE.md) |
| 知道按键怎么用 | [KEYBINDINGS.md](KEYBINDINGS.md) |
| 开发一个新功能 | [guide/NEW_FEATURE_GUIDE.md](guide/NEW_FEATURE_GUIDE.md) |
| 调试问题 | [guide/DEBUG.md](guide/DEBUG.md) |

## 📁 代码定位速查

| 功能模块 | 代码位置 | 相关文档 |
|---------|---------|---------|
| Agent 生命周期 | `Core/MAAgentManager` | ARCHITECTURE.md §6 |
| 命令系统 (RTS) | `Core/MACommandManager` | ARCHITECTURE.md §2.3 |
| 选择/编组 | `Core/MASelectionManager` | ARCHITECTURE.md §2.3 |
| 编队系统 | `Core/MASquadManager` | dev-completed/feature.md |
| StateTree 任务 | `Agent/StateTree/Task/` | ARCHITECTURE.md §2.2 |
| GAS 技能 | `Agent/GAS/Ability/` | ARCHITECTURE.md §2.2 |
| 能力组件 | `Agent/Component/Capability/` | ARCHITECTURE.md §2.1 |
| 传感器 | `Agent/Component/Sensor/` | dev-completed/feature.md |
| UI 系统 | `UI/` | dev-completed/feature.md |
| 输入系统 | `Input/MAInputActions` | KEYBINDINGS.md |
| 环境实体 | `Environment/` | dev-completed/feature.md |

## 📋 文档分类

### 核心文档
- [ARCHITECTURE.md](ARCHITECTURE.md) - 系统架构、模块设计、类继承
- [KEYBINDINGS.md](KEYBINDINGS.md) - 所有按键操作说明
- [UE5_API_CHANGES.md](UE5_API_CHANGES.md) - UE5 废弃 API 迁移指南

### 开发指南
- [guide/NEW_FEATURE_GUIDE.md](guide/NEW_FEATURE_GUIDE.md) - 新功能开发流程
- [guide/DEBUG.md](guide/DEBUG.md) - 调试技巧
- [guide/CODE_STYLE.md](guide/CODE_STYLE.md) - 编码规范

### 功能文档
- [dev-completed/](dev-completed/) - 已实现功能文档
- [dev-planning/](dev-planning/) - 规划中功能文档

### 参考资料
- [bug/](bug/) - Bug 记录与解决方案
- [dev-planning/todo.md](dev-planning/todo.md) - 待开发功能列表
