# 配置文件说明

## 配置结构

```
config/
├── simulation.json      # 仿真参数 (全局配置)
├── maps/                # 地图配置 (按地图类型分离)
│   ├── Downtown.json
│   ├── SciFiModernCity.json
│   └── Port.json
└── README.md
```

**核心设计**: `simulation.json` 中只需设置 `map_type`，系统自动加载对应的地图配置文件。

---

## simulation.json - 仿真参数

### simulation
| 参数 | 说明 |
|------|------|
| `name` | 仿真名称 (仅用于标识) |
| `version` | 配置版本号 |
| `map_type` | 地图类型，对应 `maps/` 下的配置文件名 (如 `Downtown`, `SciFiModernCity`, `Port`) |
| `run_mode` | 运行模式: `edit`(运行时修改，不保存) / `modify`(标注模式，保存到文件) |
| `use_setup_ui` | 启动时显示设置界面（已弃用） |
| `use_state_tree` | 使用 StateTree 行为树（以弃用） |
| `enable_energy_drain` | 启用能量消耗 |

### spawn_settings - 生成设置 (通用)
| 参数 | 说明 |
|------|------|
| `use_player_start` | 使用 PlayerStart 位置生成 |
| `fallback_origin` | 备用生成原点 [x, y, z] |
| `spawn_radius` | 生成半径 |
| `project_to_navmesh` | 投影到导航网格 |

### server
| 参数 | 说明 |
|------|------|
| `planner_url` | 规划器后端地址 |
| `local_server_port` | 本地 HTTP 服务端口 |
| `enable_local_server` | 启用本地服务器 |
| `use_mock_data` | 使用模拟数据 (调试用) |
| `debug_mode` | 调试模式 |
| `enable_polling` | 启用任务轮询 |
| `poll_interval_seconds` | 任务轮询间隔 (秒) |
| `enable_hitl_polling` | 启用 HITL (Human-in-the-Loop) 轮询 |
| `hitl_poll_interval_seconds` | HITL 轮询间隔 (秒) |

### navigation - 手动导航路径规划
| 参数 | 说明 |
|------|------|
| `path_planner_type` | 算法类型: `MultiLayerRaycast`(多层射线扫描) / `ElevationMap`(2.5D高程图) |

#### multi_layer_raycast - 多层射线扫描配置
| 参数 | 说明 | 默认值 | 范围 |
|------|------|--------|------|
| `layer_count` | 射线层数 | 3 | 2-5 |
| `layer_distance` | 每层检测距离 (cm) | 300 | 100-500 |
| `scan_angle_range` | 扫描角度范围 (±度) | 120 | 60-180 |
| `scan_angle_step` | 扫描角度步长 (度) | 15 | 5-30 |

#### elevation_map - 2.5D 高程图配置
| 参数 | 说明 | 默认值 | 范围 |
|------|------|--------|------|
| `cell_size` | 栅格单元大小 (cm) | 100 | 50-200 |
| `search_radius` | 搜索半径 (cm) | 3000 | 1000-10000 |
| `max_slope_angle` | 最大可通行坡度角 (度) | 30 | 10-45 |
| `max_step_height` | 最大可通行台阶高度 (cm) | 50 | 20-100 |
| `path_smoothing_factor` | 路径平滑因子 | 0.15 | 0.1-0.5 |

### flight - 飞行参数 (统一配置)
| 参数 | 说明 | 默认值 |
|------|------|--------|
| `min_altitude` | 最低飞行高度 (cm) | 800 |
| `default_altitude` | 默认飞行高度 (cm) | 1000 |
| `max_speed` | 最大飞行速度 (cm/s) | 600 |
| `obstacle_detection_range` | 障碍物检测距离 (cm) | 800 |
| `obstacle_avoidance_radius` | 障碍物避让半径 (cm) | 150 |
| `acceptance_radius` | 到达判定半径 (cm) | 200 |

### ground_navigation - 地面导航参数 (统一配置)
| 参数 | 说明 | 默认值 |
|------|------|--------|
| `acceptance_radius` | 到达判定半径 (cm) | 200 |
| `stuck_timeout` | 卡住超时时间 (秒) | 10 |

### follow - 跟随参数 (统一配置，适用于所有机器人)
| 参数 | 说明 | 默认值 |
|------|------|--------|
| `distance` | 跟随距离 (cm) | 300 |
| `position_tolerance` | 跟随位置容差 (cm) - 判断是否到达跟随位置 | 350 |
| `continuous_time_threshold` | 持续跟踪时间阈值 (秒) - 达到此时间视为跟随成功 | 30 |

### guide - 引导参数 (Guide 技能)
| 参数 | 说明 | 默认值 |
|------|------|--------|
| `target_move_mode` | 被引导对象移动模式: `direct`(直接移动，无避障) / `navmesh`(导航服务，有避障) | navmesh |
| `wait_distance_threshold` | 等待距离阈值 (cm) - 机器人与被引导对象距离超过此值时停下等待 | 500 |

### handle_hazard - 处理危险参数 (HandleHazard 技能)
| 参数 | 说明 | 默认值 |
|------|------|--------|
| `safe_distance` | 安全距离 (cm) - 机器人与危险源保持的距离 | 1000 |
| `duration` | 处理持续时间 (秒) | 15 |
| `spray_speed` | 喷射特效移动速度 (cm/s) | 800 |
| `spray_width` | 喷射特效宽度 (倍数) | 1.0 |

### take_photo - 拍照参数 (TakePhoto 技能)
| 参数 | 说明 | 默认值 |
|------|------|--------|
| `photo_distance` | 拍照距离 (cm) - 机器人与目标的距离 | 800 |
| `photo_duration` | 拍照持续时间 (秒) | 15 |
| `camera_fov` | 相机视场角 (度) | 60 |
| `camera_forward_offset` | 相机前向偏移 (cm) | 50 |

### broadcast - 喊话参数 (Broadcast 技能)
| 参数 | 说明 | 默认值 |
|------|------|--------|
| `broadcast_distance` | 喊话距离 (cm) - 机器人与目标的距离 | 800 |
| `broadcast_duration` | 喊话持续时间 (秒) | 20 |
| `effect_speed` | 声波特效移动速度 (cm/s) | 1000 |
| `effect_width` | 声波特效宽度 (cm) | 1000 |
| `shock_rate` | 声波震动频率 (Hz) | 3.0 |

---

## maps/{MapType}.json - 地图配置

每个地图类型一个配置文件，包含该地图特有的设置。

### map_info - 地图信息
| 参数 | 说明 |
|------|------|
| `display_name` | 显示名称 |
| `map_path` | UE 地图资源路径 |
| `scene_graph_folder` | 场景图文件夹路径 (相对于 unreal_project) |

### spectator - 观察者相机
| 参数 | 说明 |
|------|------|
| `position` | 初始相机位置 [x, y, z] |
| `rotation` | 初始相机旋转 [pitch, yaw, roll] |

### agents - 机器人配置
```json
{
  "agents": [
    {
      "type": "UAV/UGV/Quadruped/Humanoid",
      "instances": [
        {
          "label": "UAV-1",
          "position": [x, y, z],
          "rotation": [pitch, yaw, roll],
          "battery_level": 100
        }
      ]
    }
  ]
}
```

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `type` | 机器人类型: `UAV`, `UGV`, `Quadruped`, `Humanoid` | - |
| `label` | 显示标签 | - |
| `position` | 生成位置 [x, y, z] | - |
| `rotation` | 生成朝向 [pitch, yaw, roll] | [0, 0, 0] |
| `battery_level` | 初始电量 (0-100) | 100 |

### environment - 环境配置

环境对象统一使用数组格式，每个对象包含以下通用字段：

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `label` | 显示标签 (唯一标识) | - |
| `type` | 对象类型 (见下表) | - |
| `position` | 位置 [x, y, z] | - |
| `rotation` | 朝向 [pitch, yaw, roll] | [0, 0, 0] |
| `enabled` | 是否启用 | true |
| `features` | 特征属性 (键值对) | {} |
| `patrol` | 巡逻配置 (可选) | - |

#### 支持的环境对象类型

| type | 说明 | 特有 features |
|------|------|---------------|
| `charging_station` | 充电站 | - |
| `cargo` | 可拾取货物 | `color`, `subtype` |
| `person` | 行人 | `subtype`, `variant`, `gender`, `animation`, `clothing_color`, `crowd` |
| `vehicle` | 车辆 | `subtype`, `color`, `is_fire`, `fire_scale`, `traffic_violation` |
| `boat` | 船只 | `subtype`, `is_fire`, `is_spill` |
| `fire` | 火焰特效 | `scale` |
| `smoke` | 烟雾特效 | `scale`, `radius` |
| `wind` | 强风特效 | `scale`, `radius` |
| `assembly_component` | 组装组件 | `subtype` |

#### features 详细说明

**cargo (货物)**
| 特征 | 说明 | 可选值 |
|------|------|--------|
| `color` | 颜色 | `red`, `blue`, `green`, `yellow`, 等 |
| `subtype` | 子类型 | `box` |

**person (行人)**
| 特征 | 说明 | 可选值 |
|------|------|--------|
| `subtype` | 穿着风格 | `business`, `casual`, `sportive` |
| `variant` | 变体编号 | `01`, `02`, `03`, ... |
| `gender` | 性别 | `male`, `female` |
| `animation` | 动画状态 | `idle`, `walk`, `talk` |
| `clothing_color` | 服装颜色 | `yellow`, `beige`, `pink`, `brown`, 等 |
| `crowd` | 是否为人群成员 | `true`, `false` |

**vehicle (车辆)**
| 特征 | 说明 | 可选值 |
|------|------|--------|
| `subtype` | 车型 | `sedan`, `suv`, `hatchback`, `crossover`, `wagon`, `sporty`, `van` |
| `color` | 颜色 | `red`, `blue`, `silver`, `black`, `gray`, `orange`, `white` |
| `is_fire` | 是否着火 | `true`, `false` |
| `fire_scale` | 火焰大小 | 数字字符串，如 `"8"` |
| `traffic_violation` | 是否违章 | `true`, `false` |

**boat (船只)**
| 特征 | 说明 | 可选值 |
|------|------|--------|
| `subtype` | 船型 | `lifeboat`, `metal_03`, 等 |
| `is_fire` | 是否着火 | `true`, `false` |
| `is_spill` | 是否泄漏 | `true`, `false` |

**fire/smoke/wind (特效)**
| 特征 | 说明 |
|------|------|
| `scale` | 特效缩放比例 |
| `radius` | 影响半径 (cm) - 仅 smoke/wind |

**assembly_component (组装组件)**
| 特征 | 说明 | 可选值 |
|------|------|--------|
| `subtype` | 组件类型 | `solar_panel`, `antenna_module`, `address_speaker`, `stand` |

#### patrol - 巡逻配置

支持 `person`, `vehicle`, `boat` 类型的对象进行巡逻。

```json
{
  "patrol": {
    "enabled": true,
    "waypoints": [
      [x1, y1, z1],
      [x2, y2, z2]
    ],
    "loop": true,
    "wait_time": 2.0
  }
}
```

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `enabled` | 是否启用巡逻 | false |
| `waypoints` | 巡逻路径点数组 [[x,y,z], ...] | [] |
| `loop` | 是否循环巡逻 | false |
| `wait_time` | 每个路径点等待时间 (秒) | 0 |

---

## 切换地图

只需修改 `simulation.json` 中的 `map_type` 字段：

```json
{
  "simulation": {
    "map_type": "SciFiModernCity"
  }
}
```

系统会自动加载 `maps/SciFiModernCity.json` 中的所有地图相关配置。
