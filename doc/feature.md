# 功能特性文档

## Robot 共用参数 (CARLA 风格)

本项目采用 CARLA 风格架构，参数存储在 Robot 上，多个 Task/Ability 共用。

### MARobotDogCharacter 属性

| 属性 | 类型 | 默认值 | 用途 |
|------|------|--------|------|
| `ScanRadius` | float | 200 | 感知范围（多任务复用） |
| `FollowTarget` | AMACharacter* | null | 跟随目标 |
| `CoverageArea` | AActor* | null | 覆盖区域 |
| `PatrolPath` | AMAPatrolPath* | null | 巡逻路径 |

### ScanRadius 复用

| Task | 用途 |
|------|------|
| MA Follow | 与目标保持的距离 |
| MA Coverage | 路径间距 + 到达判定半径 |
| MA Patrol | 到达路径点判定半径 |

### 设计好处

- 参数集中管理，改一处全生效
- 运行时可动态修改
- 符合物理意义（感知范围）

---

## 区域覆盖 (Coverage)

### 概述

区域覆盖功能让机器人按"割草机"模式扫描指定区域，确保完整覆盖。

### CARLA 风格设计

参数存储在 Robot 上，Task 从 Robot 获取：

```cpp
// Robot 属性
Robot->CoverageArea;   // 覆盖区域
Robot->ScanRadius;     // 扫描半径（复用）
```

### 组件

- `MACoverageArea` - 定义覆盖区域边界（Spline）
- `MASTTask_Coverage` - StateTree Task，执行覆盖任务

### 路径生成

1. 从 `Robot->GetCoverageArea()` 获取覆盖区域
2. 使用 `Robot->ScanRadius` 计算路径间距
3. NavMesh 投影确保所有点可达
4. 投影失败时尝试4个方向偏移（1x, 2x, 3x 半径）

### 可视化

- 橙色线：区域边界
- 绿色点和线：实际执行路径（任务开始后显示）
- 绿色球：起点
- 红色球：终点

### 使用方式

```cpp
// 1. 设置覆盖区域（必须）
Robot->SetCoverageArea(CoverageAreaActor);

// 2. 发送命令
ASC->AddLooseGameplayTag("Command.Coverage");
// 或按 K 键
```

### StateTree 配置

MA Coverage Task 无需配置参数，自动从 Robot 获取

---

## 跟随 (Follow)

### 概述

任何 RobotDog 都可以执行跟随命令，持续跟随指定目标。

### CARLA 风格设计

参数存储在 Robot 上，Task 从 Robot 获取：

```cpp
// Robot 属性
Robot->FollowTarget;   // 跟随目标
Robot->ScanRadius;     // 跟随距离（复用）
```

### 使用方式

```cpp
// 1. 设置跟随目标（必须）
Robot->SetFollowTarget(HumanCharacter);

// 2. 发送命令
ASC->AddLooseGameplayTag("Command.Follow");
// 或按 F 键
```

### StateTree 配置

MA Follow Task 无需配置参数，自动从 Robot 获取

---

## 充电 (Charge)

### 概述

机器人自动查找最近的充电站，导航过去并渐进式充电。

### CARLA 风格设计

Charge 比较特殊：
- 自动查找最近的充电站（不需要预设）
- `AcceptanceRadius` 从 `ChargingStation->InteractionRadius` 获取

```cpp
// 充电站属性
ChargingStation->InteractionRadius;  // 交互范围（到达判定）
```

### 渐进式充电

充电不是瞬间完成，而是渐进式恢复：
- 默认充电速率：每秒 20% 电量
- 更新间隔：0.5 秒
- 离开充电站范围会中断充电

```cpp
// GA_Charge 属性
ChargeRatePerSecond = 20.f;    // 每秒恢复 20% 电量
ChargeUpdateInterval = 0.5f;   // 每 0.5 秒更新一次
```

### 使用方式

```cpp
// 直接发送命令，自动查找最近充电站
ASC->AddLooseGameplayTag("Command.Charge");
// 或按 H 键
// 或低电量时自动触发
```

### StateTree 配置

MA Charge Task 无需配置参数，自动查找充电站并获取其 InteractionRadius

---

## 充电站 Actor (ChargingStation)

### 碰撞设计

```
AMAChargingStation
├── MeshComponent (视觉)
│   ├── SetCanEverAffectNavigation(false) - 不影响 NavMesh
│   └── SetCollisionEnabled(NoCollision) - 不阻挡移动
│
└── InteractionSphere (检测)
    ├── 半径由 InteractionRadius 属性控制（默认 300）
    └── Overlap 模式 - 可穿过但触发事件
```

### 可配置属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `InteractionRadius` | 300 | 交互范围，也用于 Charge Task 的到达判定 |

### 设计原因

| 问题 | 解决方案 |
|------|----------|
| 充电站挖掉 NavMesh，导致无法导航 | `SetCanEverAffectNavigation(false)` |
| 充电站阻挡机器人移动 | `SetCollisionEnabled(NoCollision)` |
| 需要检测机器人进入范围 | `InteractionSphere` 使用 Overlap 模式 |

---

## 巡逻 (Patrol)

### 概述

机器人循环巡逻指定路径点。

### CARLA 风格设计

参数存储在 Robot 上，Task 从 Robot 获取：

```cpp
// Robot 属性
Robot->PatrolPath;     // 巡逻路径
Robot->ScanRadius;     // 到达判定半径（复用）
```

### 使用方式

```cpp
// 1. 设置巡逻路径（必须）
Robot->SetPatrolPath(PatrolPathActor);

// 2. 发送命令
ASC->AddLooseGameplayTag("Command.Patrol");
// 或按 G 键
```

### StateTree 配置

MA Patrol Task 无需配置参数，自动从 Robot 获取

---

## 巡逻路径 Actor (PatrolPath)

### 可视化功能

运行时显示：
- 黄色球体标记每个路径点
- 青色线条连接路径点
- 方向箭头显示巡逻方向
- 绿色圆圈标记起点
- 每个点上方显示编号 (1, 2, 3...)

### 可配置属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `bShowPathAtRuntime` | true | 是否在运行时显示 |
| `PathColor` | Cyan | 路径线颜色 |
| `WaypointColor` | Yellow | 路径点颜色 |
| `WaypointSize` | 30 | 路径点大小 |
| `PathThickness` | 3 | 线条粗细 |
| `bShowWaypointNumbers` | true | 是否显示编号 |
