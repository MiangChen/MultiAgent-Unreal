# 配置文件说明

## simulation.json - 仿真参数

### simulation
| 参数 | 说明 |
|------|------|
| `run_mode` | 运行模式: `edit`(运行时修改场景，不保存) / `modify`(标注语义标签，保存到文件) |
| `use_setup_ui` | 启动时显示设置界面 |
| `use_state_tree` | 使用 StateTree 行为树 |
| `enable_energy_drain` | 启用能量消耗 |
| `default_map` | 默认地图路径 |
| `scene_graph_path` | 场景图文件路径，`edit`模式需要有`scene_graph.json`文件路径，`modify`模式可以是文件夹路径（此时完全从零开始创建`scene_graph.json`），也可以是已有`scene_graph.json`的路径（此时会增量更新内容） |

### server
| 参数 | 说明 |
|------|------|
| `planner_url` | 规划器后端地址 |
| `local_server_port` | 本地 HTTP 服务端口 |
| `enable_local_server` | 启用本地服务器 |
| `enable_polling` | 启用轮询 |
| `poll_interval_seconds` | 轮询间隔(秒) |

### spectator
| 参数 | 说明 |
|------|------|
| `position` | 初始相机位置 [x, y, z] |
| `rotation` | 初始相机旋转 [pitch, yaw, roll] |

### spawn_settings
| 参数 | 说明 |
|------|------|
| `use_player_start` | 使用 PlayerStart 位置生成 |
| `spawn_radius` | 生成半径 |
| `project_to_navmesh` | 投影到导航网格 |

---

## agents.json - 机器人配置

```json
{
  "agents": [
    {
      "type": "UAV/UGV/Quadruped/Humanoid",
      "count": 2,
      "instances": [
        { "label": "UAV_01", "position": [x, y, z], "rotation": [p, y, r] }
      ]
    }
  ]
}
```

---

## environment.json - 环境配置

### charging_stations - 充电站
| 参数 | 说明 |
|------|------|
| `id` | 唯一标识 |
| `label` | 显示名称 |
| `position` | 位置 [x, y, z] |

### objects - 可拾取物品
| 参数 | 说明 |
|------|------|
| `id` | 唯一标识 |
| `label` | 显示名称 |
| `type` | 类型 (如 `cargo`) |
| `position` | 位置 [x, y, z] |
| `features` | 特征属性 (color, subtype 等) |
