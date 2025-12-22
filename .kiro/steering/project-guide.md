# 项目导航指南

## 文档入口
- 文档索引: #[[file:doc/README.md]]
- 系统架构: #[[file:doc/ARCHITECTURE.md]]
- 按键说明: #[[file:doc/KEYBINDINGS.md]]
- 新功能开发: #[[file:doc/guide/NEW_FEATURE_GUIDE.md]]

## UE5 项目结构
C++ 源码位于 `unreal_project/Source/MultiAgent/`:
- `Core/` - Manager 子系统 (AgentManager, CommandManager, SelectionManager)
- `Agent/Character/` - Agent 角色类
- `Agent/GAS/` - GAS 技能系统
- `Agent/StateTree/` - StateTree 任务和条件
- `Agent/Component/` - 能力组件和传感器
- `UI/` - Widget 界面
- `Input/` - 输入系统
- `Environment/` - 环境实体 (充电站、巡逻路径)

## 编译
#[[file:compile.sh]]

## UE5 API 变更
#[[file:doc/UE5_API_CHANGES.md]]
