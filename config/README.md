# Configuration Reference

## Directory Structure

```
config/
├── simulation.json      # Simulation parameters (global config)
├── maps/                # Map configs (per map type)
│   ├── Downtown.json
│   ├── SciFiModernCity.json
│   └── Port.json
└── README.md
```

**Core design**: Set `map_type` in `simulation.json`, and the system automatically loads the corresponding map config file.

---

## simulation.json - Simulation Parameters

### simulation
| Parameter | Description |
|-----------|-------------|
| `name` | Simulation name (for identification only) |
| `version` | Config version number |
| `map_type` | Map type, corresponds to a config filename under `maps/` (e.g. `Downtown`, `SciFiModernCity`, `Port`) |
| `run_mode` | Run mode: `edit` (runtime changes, not saved) / `modify` (annotation mode, saved to file) |
| `use_setup_ui` | Show setup UI on startup (deprecated) |
| `use_state_tree` | Use StateTree behavior tree (deprecated) |
| `enable_energy_drain` | Enable energy consumption |
| `enable_info_checks` | Enable Info-level condition checks (e.g. high-priority target discovery) |

### spawn_settings - Spawn Settings (Global)
| Parameter | Description |
|-----------|-------------|
| `use_player_start` | Use PlayerStart location for spawning |
| `fallback_origin` | Fallback spawn origin [x, y, z] |
| `spawn_radius` | Spawn radius |
| `project_to_navmesh` | Project spawn location to navigation mesh |

### server
| Parameter | Description |
|-----------|-------------|
| `planner_url` | Planner backend URL |
| `local_server_port` | Local HTTP server port |
| `enable_local_server` | Enable local HTTP server |
| `use_mock_data` | Use mock data (for debugging) |
| `debug_mode` | Debug mode |
| `enable_polling` | Enable task polling |
| `poll_interval_seconds` | Task polling interval (seconds) |
| `enable_hitl_polling` | Enable HITL (Human-in-the-Loop) polling |
| `hitl_poll_interval_seconds` | HITL polling interval (seconds) |

### navigation - Manual Navigation Path Planning
| Parameter | Description |
|-----------|-------------|
| `path_planner_type` | Algorithm type: `MultiLayerRaycast` (multi-layer ray scanning) / `ElevationMap` (2.5D elevation map) |

#### multi_layer_raycast - Multi-Layer Ray Scanning Config
| Parameter | Description | Default | Range |
|-----------|-------------|---------|-------|
| `layer_count` | Number of ray layers | 3 | 2-5 |
| `layer_distance` | Detection distance per layer (cm) | 300 | 100-500 |
| `scan_angle_range` | Scan angle range (±degrees) | 120 | 60-180 |
| `scan_angle_step` | Scan angle step (degrees) | 15 | 5-30 |

#### elevation_map - 2.5D Elevation Map Config
| Parameter | Description | Default | Range |
|-----------|-------------|---------|-------|
| `cell_size` | Grid cell size (cm) | 100 | 50-200 |
| `search_radius` | Search radius (cm) | 3000 | 1000-10000 |
| `max_slope_angle` | Maximum traversable slope angle (degrees) | 30 | 10-45 |
| `max_step_height` | Maximum traversable step height (cm) | 50 | 20-100 |
| `path_smoothing_factor` | Path smoothing factor | 0.15 | 0.1-0.5 |

### flight - Flight Parameters (Unified Config)
| Parameter | Description | Default |
|-----------|-------------|---------|
| `min_altitude` | Minimum flight altitude (cm) | 800 |
| `default_altitude` | Default flight altitude (cm) | 1000 |
| `max_speed` | Maximum flight speed (cm/s) | 600 |
| `obstacle_detection_range` | Obstacle detection range (cm) | 800 |
| `obstacle_avoidance_radius` | Obstacle avoidance radius (cm) | 150 |
| `acceptance_radius` | Arrival acceptance radius (cm) | 200 |

### ground_navigation - Ground Navigation Parameters (Unified Config)
| Parameter | Description | Default |
|-----------|-------------|---------|
| `acceptance_radius` | Arrival acceptance radius (cm) | 200 |
| `stuck_timeout` | Stuck timeout (seconds) | 10 |

### follow - Follow Parameters (Unified Config, applies to all agents)
| Parameter | Description | Default |
|-----------|-------------|---------|
| `distance` | Follow distance (cm) | 300 |
| `position_tolerance` | Follow position tolerance (cm) - determines if the follow position is reached | 350 |
| `continuous_time_threshold` | Continuous tracking time threshold (seconds) - follow is considered successful after this duration | 30 |

### guide - Guide Parameters (Guide Skill)
| Parameter | Description | Default |
|-----------|-------------|---------|
| `target_move_mode` | Guided target movement mode: `direct` (direct movement, no avoidance) / `navmesh` (navigation service, with avoidance) | navmesh |
| `wait_distance_threshold` | Wait distance threshold (cm) - agent stops and waits when distance to guided target exceeds this value | 500 |

### handle_hazard - Hazard Handling Parameters (HandleHazard Skill)
| Parameter | Description | Default |
|-----------|-------------|---------|
| `safe_distance` | Safe distance (cm) - distance the agent maintains from the hazard source | 1000 |
| `duration` | Handling duration (seconds) | 15 |
| `spray_speed` | Spray effect movement speed (cm/s) | 800 |
| `spray_width` | Spray effect width (multiplier) | 1.0 |

### take_photo - Photo Parameters (TakePhoto Skill)
| Parameter | Description | Default |
|-----------|-------------|---------|
| `photo_distance` | Photo distance (cm) - distance between agent and target | 800 |
| `photo_duration` | Photo duration (seconds) | 15 |
| `camera_fov` | Camera field of view (degrees) | 60 |
| `camera_forward_offset` | Camera forward offset (cm) | 50 |

### broadcast - Broadcast Parameters (Broadcast Skill)
| Parameter | Description | Default |
|-----------|-------------|---------|
| `broadcast_distance` | Broadcast distance (cm) - distance between agent and target | 800 |
| `broadcast_duration` | Broadcast duration (seconds) | 20 |
| `effect_speed` | Sound wave effect speed (cm/s) | 1000 |
| `effect_width` | Sound wave effect width (cm) | 1000 |
| `shock_rate` | Sound wave oscillation frequency (Hz) | 3.0 |

---

## maps/{MapType}.json - Map Configuration

Each map type has its own config file containing map-specific settings.

### map_info - Map Information
| Parameter | Description |
|-----------|-------------|
| `display_name` | Display name |
| `map_path` | UE map asset path |
| `scene_graph_folder` | Scene graph folder path (relative to unreal_project) |

### spectator - Spectator Camera
| Parameter | Description |
|-----------|-------------|
| `position` | Initial camera position [x, y, z] |
| `rotation` | Initial camera rotation [pitch, yaw, roll] |

### agents - Agent Configuration
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

| Parameter | Description | Default |
|-----------|-------------|---------|
| `type` | Agent type: `UAV`, `UGV`, `Quadruped`, `Humanoid` | - |
| `label` | Display label | - |
| `position` | Spawn position [x, y, z] | - |
| `rotation` | Spawn rotation [pitch, yaw, roll] | [0, 0, 0] |
| `battery_level` | Initial battery level (0-100) | 100 |

### environment - Environment Configuration

Environment objects use a unified array format. Each object contains the following common fields:

| Parameter | Description | Default |
|-----------|-------------|---------|
| `label` | Display label (unique identifier) | - |
| `type` | Object type (see table below) | - |
| `position` | Position [x, y, z] | - |
| `rotation` | Rotation [pitch, yaw, roll] | [0, 0, 0] |
| `enabled` | Whether the object is enabled | true |
| `features` | Feature attributes (key-value pairs) | {} |
| `patrol` | Patrol configuration (optional) | - |

#### Supported Environment Object Types

| type | Description | Specific features |
|------|-------------|-------------------|
| `charging_station` | Charging station | - |
| `cargo` | Pickable cargo | `color`, `subtype` |
| `person` | Pedestrian | `subtype`, `variant`, `gender`, `animation`, `clothing_color`, `crowd` |
| `vehicle` | Vehicle | `subtype`, `color`, `is_fire`, `fire_scale`, `traffic_violation` |
| `boat` | Boat | `subtype`, `is_fire`, `is_spill` |
| `fire` | Fire effect | `scale` |
| `smoke` | Smoke effect | `scale`, `radius` |
| `wind` | Wind effect | `scale`, `radius` |
| `assembly_component` | Assembly component | `subtype` |

#### Feature Details

**cargo**
| Feature | Description | Values |
|---------|-------------|--------|
| `color` | Color | `red`, `blue`, `green`, `yellow`, etc. |
| `subtype` | Subtype | `box` |

**person**
| Feature | Description | Values |
|---------|-------------|--------|
| `subtype` | Clothing style | `business`, `casual`, `sportive` |
| `variant` | Variant number | `01`, `02`, `03`, ... |
| `gender` | Gender | `male`, `female` |
| `animation` | Animation state | `idle`, `walk`, `talk` |
| `clothing_color` | Clothing color | `yellow`, `beige`, `pink`, `brown`, etc. |
| `crowd` | Whether part of a crowd | `true`, `false` |

**vehicle**
| Feature | Description | Values |
|---------|-------------|--------|
| `subtype` | Vehicle model | `sedan`, `suv`, `hatchback`, `crossover`, `wagon`, `sporty`, `van` |
| `color` | Color | `red`, `blue`, `silver`, `black`, `gray`, `orange`, `white` |
| `is_fire` | Whether on fire | `true`, `false` |
| `fire_scale` | Fire size | Numeric string, e.g. `"8"` |
| `traffic_violation` | Whether in traffic violation | `true`, `false` |

**boat**
| Feature | Description | Values |
|---------|-------------|--------|
| `subtype` | Boat model | `lifeboat`, `metal_03`, etc. |
| `is_fire` | Whether on fire | `true`, `false` |
| `is_spill` | Whether spilling | `true`, `false` |

**fire / smoke / wind (effects)**
| Feature | Description |
|---------|-------------|
| `scale` | Effect scale |
| `radius` | Effect radius (cm) - smoke/wind only |

**assembly_component**
| Feature | Description | Values |
|---------|-------------|--------|
| `subtype` | Component type | `solar_panel`, `antenna_module`, `address_speaker`, `stand` |

#### patrol - Patrol Configuration

Supports patrol for `person`, `vehicle`, and `boat` object types.

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

| Parameter | Description | Default |
|-----------|-------------|---------|
| `enabled` | Whether patrol is enabled | false |
| `waypoints` | Patrol waypoint array [[x,y,z], ...] | [] |
| `loop` | Whether to loop the patrol | false |
| `wait_time` | Wait time at each waypoint (seconds) | 0 |

---

## Switching Maps

Simply change the `map_type` field in `simulation.json`:

```json
{
  "simulation": {
    "map_type": "Downtown"
  }
}
```

The system will automatically load all map-related configuration from `maps/Downtown.json`.
