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
| `map_type` | 地图类型，对应 `maps/` 下的配置文件名 (如 `Downtown`, `SciFiModernCity`, `Port`) |
| `run_mode` | 运行模式: `edit`(运行时修改，不保存) / `modify`(标注模式，保存到文件) |
| `use_setup_ui` | 启动时显示设置界面 |
| `use_state_tree` | 使用 StateTree 行为树 |
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
| `enable_polling` | 启用轮询 |
| `poll_interval_seconds` | 轮询间隔(秒) |

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
        { "label": "UAV_01", "position": [x, y, z], "rotation": [p, y, r] }
      ]
    }
  ]
}
```

### environment - 环境配置

#### charging_stations - 充电站
| 参数 | 说明 |
|------|------|
| `id` | 唯一标识 |
| `label` | 显示名称 |
| `position` | 位置 [x, y, z] |

#### objects - 可拾取物品
| 参数 | 说明 |
|------|------|
| `id` | 唯一标识 |
| `label` | 显示名称 |
| `type` | 类型 (如 `cargo`) |
| `position` | 位置 [x, y, z] |
| `features` | 特征属性 (color, subtype 等) |

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
