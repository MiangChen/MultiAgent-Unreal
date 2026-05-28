# 项目导航指南

## 文档入口
- 文档索引: #[[file:doc/README.md]]
- 系统架构: #[[file:doc/ARCHITECTURE.md]]
- 按键说明: #[[file:doc/KEYBINDINGS.md]]
- 新功能开发: #[[file:doc/guide/NEW_FEATURE_GUIDE.md]]

## UE5 项目结构
C++ 源码位于 `unreal_project/Source/MultiAgent/`:
- `Core/` - 核心子系统
  - `Config/` - 配置管理 (MAConfigManager - 统一配置加载)
  - `GameFlow/` - 游戏流程 (MAGameInstance, MAGameMode, MASetupGameMode)
  - `Manager/` - 管理器 (AgentManager, CommandManager, SelectionManager, SquadManager, ViewportManager, EmergencyManager)
  - `Comm/` - 通信系统 (MACommSubsystem, MACommTypes)
  - `Types/` - 类型定义 (MATypes, MASimTypes, MATaskGraphTypes)
- `Agent/Character/` - Agent 角色类 (UAV, FixedWingUAV, UGV, Quadruped, Humanoid)
- `Agent/Skill/` - 技能系统（技能管理层 + 技能内核层）
- `Agent/Skill/Impl/` - 技能实现 (SK_Navigate, SK_Follow, SK_Charge, SK_Place)
- `Agent/Skill/Utils/` - 辅助工具 (几何计算, 场景查询)
- `Agent/StateTree/` - StateTree 任务和条件
- `Agent/Component/` - 传感器组件
- `UI/` - Widget 界面
- `Input/` - 输入系统
- `Environment/` - 环境实体 (充电站)
- `Utils/` - 全局工具 (MAWorldQuery - 世界状态查询)

## Python 脚本
- `scripts/send_skill_list.py` - 发送技能列表并接收反馈
- `scripts/world_query.py` - 世界状态查询客户端

## 配置文件
- `config/simulation.json` - 仿真参数
- `config/agents.json` - 机器人配置
- `config/environment.json` - 环境配置

## 机器人类型
| 类型 | 技能 |
|------|------|
| UAV | Navigate, Search, Follow |
| FixedWingUAV | Navigate, Search |
| UGV | Navigate, Carry |
| Quadruped | Navigate, Search, Follow |
| Humanoid | Navigate, Place |

## 编译
#[[file:compile.sh]]

## UE5 API 变更
#[[file:doc/UE5_API_CHANGES.md]]
