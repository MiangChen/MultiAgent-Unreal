# TODO List

## 已完成

### Phase 1: 核心框架 ✅
- [x] UMAActorSubsystem - Character 生命周期管理
- [x] AMACharacter 及其子类
- [x] GAS 集成 - AbilitySystemComponent
- [x] 所有 Ability (Pickup, Drop, Navigate, Follow, Search, etc.)
- [x] State Tree 基础集成

### Phase 2: Sensor Component 重构 ✅
- [x] Sensor 从 Actor 改为 Component 模式
- [x] UMASensorComponent 基类
- [x] UMACameraSensorComponent (拍照/录像/TCP流)
- [x] MACharacter Sensor 管理 API

### Phase 3: AgentManager + Action 动态发现 ✅
- [x] MAActorSubsystem → MAAgentManager 重命名
- [x] MATypes.h 公共类型定义 (避免循环依赖)
- [x] JSON 配置驱动 Agent 创建 (config/agents.json)
- [x] Action 动态发现机制 (GetAvailableActions + ExecuteAction)
- [x] MASensorComponent Action 接口
- [x] MACameraSensorComponent 6 个 Actions
- [x] MACharacter Action 聚合 + 6 个 Agent Actions

### Phase 4: Interface 架构重构 ✅
- [x] 创建 MAAgentInterfaces.h 定义 4 个 Interface
- [x] MARobotDogCharacter 实现 IMAPatrollable, IMAFollowable, IMACoverable, IMAChargeable
- [x] MADroneCharacter 实现 IMAPatrollable, IMAFollowable, IMACoverable, IMAChargeable
- [x] StateTree Tasks 使用 Interface 而非硬编码类型
- [x] MAPlayerController 支持 Drone 所有技能

### Phase 5: 选择与编队系统 ✅
- [x] MASelectionManager - 框选、点选、编组
- [x] MACommandManager - 命令分发
- [x] MASquadManager + UMASquad - Squad 系统
- [x] 星际争霸风格编组快捷键 (1-5, Ctrl+1-5)
- [x] Squad 创建/解散 (Q, Shift+Q)
- [x] 编队类型切换 (B)

---

## 待开发

### 高优先级

#### 智能体类型扩展
- [ ] 实现更多无人机型号（按需添加）

### 中优先级

#### 配置系统
- [ ] Agent 配置热重载支持（运行时重新加载 JSON 配置）
- [ ] Sensor 配置热重载

#### 可视化工具
- [ ] Agent 配置可视化编辑器（UE Editor 插件）
- [ ] Sensor 配置可视化编辑器
- [ ] 编队可视化预览工具

#### 更多 Sensor 类型
- [ ] Lidar 传感器
- [ ] Depth 深度传感器
- [ ] IMU 惯性测量单元

### 低优先级

#### Manager 扩展
- [ ] UMARelationSubsystem - 实体关系图
- [ ] MapManager - 地图感知

#### Mass AI 集成
- [ ] 完成 Mass AI ECS 架构迁移（支持 100+ 智能体）
- [ ] Mass AI 与 GAS 集成

#### Python API
- [ ] 实现 Python 远程控制 API
- [ ] Agent Action 远程调用接口

#### 其他
- [ ] SaveGame 系统 - 游戏存档/读档

---

*最后更新: 2025-12-08*
