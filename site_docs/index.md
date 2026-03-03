# 功能性介绍

本页用于对外快速说明项目能力边界：支持什么、可配置什么、如何接入。

## 1. 项目定位

MultiAgent-Unreal 是基于 UE5 的多机器人协同仿真平台，支持通过本地 Web 控制台和 HTTP 接口向仿真端下发任务（任务图、技能分配、技能列表），并在 UE 内完成审核与执行。

## 2. 支持系统

| 系统 | 状态 | 说明 |
|---|---|---|
| macOS | ✅ 推荐 | 提供 `mac_compile_and_start.sh` 一键编译启动脚本 |
| Linux | ✅ 支持 | 提供 `example_compile.sh` / `example_start_linux.sh` 示例脚本 |
| Windows | ⚠️ 可运行 | UE5 本身支持，仓库暂未提供同等一键脚本 |

## 3. 支持机器人类型

机器人类型在 `config/maps/*.json` 的 `agents[].type` 中配置，当前代码包含以下类型：

- `UAV`
- `FixedWingUAV`
- `UGV`
- `Quadruped`
- `Humanoid`

## 4. 支持技能（按机器人）

当前技能集合来自各机器人角色初始化逻辑（`InitializeSkillSet`）：

| 机器人类型 | 已支持技能 |
|---|---|
| UAV | Navigate, Search, Follow, Charge |
| FixedWingUAV | Navigate, Search, Charge |
| UGV | Navigate, Carry, Charge |
| Quadruped | Navigate, Search, Follow, Charge |
| Humanoid | Navigate, Place, Charge |

## 5. Web 端能力

内置 mock backend（`scripts/mock_backend.py`）提供 Web 控制台（默认 `http://localhost:8081`）：

- 预设 `Task Graph` 下发（HITL 流程）
- 预设 `Skill Allocation` 下发（HITL 流程）
- 预设 `Skill List` 下发（直接执行）
- UE5 连接状态显示（未连接/已连接）
- 双向通信日志展示（Backend -> UE5、UE5 -> Backend）

## 6. 可配置能力

## 6.1 机器人数量、品种、初始状态

配置文件：`config/maps/{MapType}.json`

通过 `agents[].instances[]` 增删条目即可调节机器人数量；通过 `type` 决定品种。

```json
{
  "type": "UAV",
  "instances": [
    { "label": "UAV-1", "position": [7700, 9000, 20], "rotation": [0, 180, 0], "battery_level": 100 }
  ]
}
```

## 6.2 地图加载与自定义场景

- 在 `config/simulation.json` 里切换 `simulation.map_type`
- 在 `config/maps/{MapType}.json` 里配置：
  - `map_info.map_path`：UE 地图资源路径
  - `map_info.scene_graph_folder`：对应语义场景图目录

因此，加载“自己的场景”通常是新增一个 `{MapType}.json` 并把 `map_type` 切过去。

## 6.3 环境对象配置

同样在 `config/maps/{MapType}.json` 的 `environment[]` 中配置。
支持对象如 `charging_station`、`cargo`、`person`、`vehicle`、`boat`、`fire`、`smoke`、`wind` 等。

## 7. 导航与路径配置

## 7.1 全局路径规划器参数

配置文件：`config/simulation.json`

- `navigation.path_planner_type`：`MultiLayerRaycast` / `ElevationMap`
- 对应参数块：`navigation.multi_layer_raycast` 或 `navigation.elevation_map`

## 7.2 巡逻路径（Waypoints）

可在环境对象里配置 `patrol`（常用于 person/vehicle/boat）：

```json
"patrol": {
  "enabled": true,
  "waypoints": [[x1, y1, z1], [x2, y2, z2]],
  "loop": true,
  "wait_time": 1.0
}
```

## 7.3 机器人任务路径

机器人导航目标通常通过任务消息下发（例如 `navigate` 的目标点），可由 Web 预设按钮或外部 API 发送。
