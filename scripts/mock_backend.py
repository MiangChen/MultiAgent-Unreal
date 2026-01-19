#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Mock Backend Server with Web UI
For testing the communication module of MultiAgent-Unreal simulation platform

Features:
- Web UI at http://localhost:8080
- Left panel: Skill list buttons and Task Graph buttons to send commands to UE5
- Right panel: Real-time display of messages from UE5
- WebSocket for real-time updates

API Endpoints:
- GET  /                     - Web UI
- GET  /api/sim/poll         - UE5 polls for pending messages
- POST /api/sim/message      - Receive messages from UE5
- POST /api/sim/scene_change - Receive scene change messages
- POST /api/send_skill       - Send skill list (from Web UI)
- POST /api/send_task_graph  - Send task graph (from Web UI)
- GET  /api/messages         - Get received messages (SSE stream)

Usage:
    python3 scripts/mock_backend.py [--port 8080]
"""

import json
import argparse
import threading
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime
from urllib.parse import urlparse, parse_qs
import sys
import uuid
import queue

# ========== 配置 ==========
DEFAULT_REDBOX_POSITION = {"x": 300, "y": 2400, "z": 0}

# ========== 全局状态 ==========
pending_messages = []
message_lock = threading.Lock()
message_queue = queue.Queue()  # For SSE streaming

# ========== 技能列表定义 ==========
SKILL_LISTS = {
    "complete_test": {
        "name": "Complete Test",
        "description": "完整测试: UAV 搜索 → 导航装载 → 运输卸载",
        "data": {
            "0": {
                "UAV_01": {"skill": "take_off", "params": {}},
                "UAV_02": {"skill": "take_off", "params": {}}
            },
            "1": {
                "UAV_01": {"skill": "search", "params": {
                    "search_area": [[-500, 1000], [1500, 1000], [1500, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }},
                "UAV_02": {"skill": "search", "params": {
                    "search_area": [[-1000, 1000], [-2000, 1000], [-2000, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }}
            },
            "2": {
                "UGV_01": {"skill": "navigate", "params": {"dest": {"x": "redbox_x + 100", "y": "redbox_y - 400", "z": 0}}},
                "Humanoid_01": {"skill": "navigate", "params": {"dest": {"x": "redbox_x - 100", "y": "redbox_y - 200", "z": 0}}},
                "UAV_01": {"skill": "return_home", "params": {}},
                "UAV_02": {"skill": "return_home", "params": {}}
            },
            "3": {
                "Humanoid_01": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "robot", "type": "UGV", "features": {"label": "UGV_01"}}
                }}
            },
            "4": {
                "UGV_01": {"skill": "navigate", "params": {"dest": {"x": -5100, "y": 2200, "z": 0}}},
                "Humanoid_01": {"skill": "navigate", "params": {"dest": {"x": -5200, "y": 2100, "z": 0}}}
            },
            "5": {
                "Humanoid_01": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "ground", "type": "", "features": {}}
                }}
            }
        }
    },
    "uav_search": {
        "name": "UAV Cooperative Search",
        "description": "2个 UAV 协同搜索 RedBox",
        "data": {
            "0": {
                "UAV_01": {"skill": "take_off", "params": {}},
                "UAV_02": {"skill": "take_off", "params": {}}
            },
            "1": {
                "UAV_01": {"skill": "search", "params": {
                    "search_area": [[-500, 1000], [1500, 1000], [1500, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }},
                "UAV_02": {"skill": "search", "params": {
                    "search_area": [[-1000, 1000], [-2000, 1000], [-2000, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }}
            }
        }
    },
    "navigate_load": {
        "name": "Navigate to RedBox and Load to UGV",
        "description": "UGV 和 Humanoid 导航到 RedBox 位置，Humanoid 将 RedBox 装载到 UGV 上",
        "data": {
            "0": {
                "UGV_01": {"skill": "navigate", "params": {"dest": {"x": "redbox_x + 100", "y": "redbox_y - 400", "z": 0}}},
                "Humanoid_01": {"skill": "navigate", "params": {"dest": {"x": "redbox_x - 100", "y": "redbox_y - 200", "z": 0}}},
                "UAV_01": {"skill": "return_home", "params": {}},
                "UAV_02": {"skill": "return_home", "params": {}}
            },
            "1": {
                "Humanoid_01": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "robot", "type": "UGV", "features": {"label": "UGV_01"}}
                }}
            }
        }
    },
    "transport_unload": {
        "name": "Transport to Destination and Unload",
        "description": "UGV 和 Humanoid 导航到目的地，Humanoid 将 RedBox 从 UGV 卸载到地面",
        "data": {
            "0": {
                "UGV_01": {"skill": "navigate", "params": {"dest": {"x": -5100, "y": 2200, "z": 0}}},
                "Humanoid_01": {"skill": "navigate", "params": {"dest": {"x": -5200, "y": 2100, "z": 0}}}
            },
            "1": {
                "Humanoid_01": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "ground", "type": "", "features": {}}
                }}
            }
        }
    },
    "single": {
        "name": "Single Test",
        "description": "UAV 起飞 -> 导航 -> Humanoid 导航",
        "data": {
            "0": {"UAV_01": {"skill": "take_off", "params": {}}},
            "1": {"UAV_01": {"skill": "navigate", "params": {"dest": {"x": 1350, "y": -450, "z": 545}}}},
            "2": {"Humanoid_01": {"skill": "navigate", "params": {"dest": {"x": -1200, "y": 1200, "z": 0}}}},
            "3": {"Humanoid_01": {"skill": "navigate", "params": {"dest": {"x": 0, "y": 2400, "z": 0}}}}
        }
    },
    "place_coop": {
        "name": "Place Coop",
        "description": "UGV + Humanoid 协作搬运",
        "data": {
            "0": {
                "UGV_01": {"skill": "navigate", "params": {"dest": {"x": 400, "y": 1300, "z": 0}}},
                "Humanoid_01": {"skill": "navigate", "params": {"dest": {"x": 200, "y": 1850, "z": 0}}}
            },
            "1": {
                "Humanoid_01": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "robot", "type": "UGV", "features": {"label": "UGV_01"}}
                }}
            },
            "2": {"UGV_01": {"skill": "navigate", "params": {"dest": {"x": 1000, "y": 1000, "z": 0}}}},
            "3": {"Humanoid_01": {"skill": "navigate", "params": {"dest": {"x": 1150, "y": 1000, "z": 0}}}},
            "4": {
                "Humanoid_01": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "ground", "type": "", "features": {}}
                }}
            }
        }
    }
}

# ========== 预设任务图定义 ==========
TASK_GRAPHS = {
    "fire_rescue": {
        "name": "Fire Rescue Mission",
        "description": "派遣无人机侦察火灾区域，机器狗前往灭火，人形机器人疏散人员",
        "data": {
            "meta": {
                "description": "派遣无人机侦察火灾区域，机器狗前往灭火，人形机器人疏散人员。",
                "shared_skill_groups": [["T1.0", "T4.0"], ["T2.0", "T5.0"], ["T3.0", "T6.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "无人机飞往火灾区域进行空中侦察", "location": "building_A_roof",
                     "required_skills": [{"skill_name": "navigate_to(building_A_roof)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["drone_at_fire_zone"]},
                    {"task_id": "T2", "description": "无人机扫描火灾区域获取热成像数据", "location": "building_A_roof",
                     "required_skills": [{"skill_name": "perceive(thermal_scan)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["fire_location_data", "victim_locations"]},
                    {"task_id": "T3", "description": "机器狗导航至火灾现场入口", "location": "building_A_entrance",
                     "required_skills": [{"skill_name": "navigate_to(building_A_entrance)", "assigned_robot_type": ["RobotDog"], "assigned_robot_count": 2}],
                     "produces": ["dogs_at_entrance"]},
                    {"task_id": "T4", "description": "无人机广播火灾位置给地面机器人", "location": "building_A_roof",
                     "required_skills": [{"skill_name": "broadcast(fire_location_data)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["fire_info_shared"]},
                    {"task_id": "T5", "description": "机器狗执行灭火操作", "location": "tbd:fire_point",
                     "required_skills": [{"skill_name": "manipulate(fire_extinguisher)", "assigned_robot_type": ["RobotDog"], "assigned_robot_count": 2}],
                     "produces": ["fire_suppressed"]},
                    {"task_id": "T6", "description": "人形机器人疏散被困人员", "location": "tbd:victim_location",
                     "required_skills": [
                         {"skill_name": "navigate_to(victim_location)", "assigned_robot_type": ["Humanoid"], "assigned_robot_count": 1},
                         {"skill_name": "manipulate(assist_evacuation)", "assigned_robot_type": ["Humanoid"], "assigned_robot_count": 1}
                     ],
                     "produces": ["victims_evacuated"]}
                ],
                "edges": [
                    {"from": "T1", "to": "T2", "type": "sequential"},
                    {"from": "T2", "to": "T4", "type": "sequential"},
                    {"from": "T2", "to": "T3", "type": "parallel"},
                    {"from": "T4", "to": "T5", "type": "conditional", "condition": "fire_info_shared != null"},
                    {"from": "T3", "to": "T5", "type": "sequential"},
                    {"from": "T2", "to": "T6", "type": "conditional", "condition": "victim_locations != null"}
                ]
            }
        }
    },
    "warehouse_patrol": {
        "name": "Warehouse Patrol",
        "description": "仓库巡逻任务：UAV 空中监控，RobotDog 地面巡逻",
        "data": {
            "meta": {
                "description": "仓库安全巡逻任务，UAV 进行空中监控，RobotDog 进行地面巡逻检查。",
                "shared_skill_groups": [["T1.0", "T2.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "UAV 起飞并飞往仓库上空", "location": "warehouse_center",
                     "required_skills": [{"skill_name": "navigate_to(warehouse_center)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["uav_at_position"]},
                    {"task_id": "T2", "description": "UAV 执行区域监控扫描", "location": "warehouse_center",
                     "required_skills": [{"skill_name": "perceive(area_scan)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["surveillance_data"]},
                    {"task_id": "T3", "description": "RobotDog 导航至仓库入口", "location": "warehouse_entrance",
                     "required_skills": [{"skill_name": "navigate_to(warehouse_entrance)", "assigned_robot_type": ["RobotDog"], "assigned_robot_count": 1}],
                     "produces": ["dog_at_entrance"]},
                    {"task_id": "T4", "description": "RobotDog 执行地面巡逻", "location": "warehouse_interior",
                     "required_skills": [{"skill_name": "patrol(warehouse_route)", "assigned_robot_type": ["RobotDog"], "assigned_robot_count": 1}],
                     "produces": ["patrol_complete"]}
                ],
                "edges": [
                    {"from": "T1", "to": "T2", "type": "sequential"},
                    {"from": "T3", "to": "T4", "type": "sequential"},
                    {"from": "T1", "to": "T3", "type": "parallel"}
                ]
            }
        }
    },
    "cargo_delivery": {
        "name": "Cargo Delivery",
        "description": "货物配送任务：UAV 搜索货物，Humanoid 装载，UGV 运输",
        "data": {
            "meta": {
                "description": "协同货物配送任务，UAV 搜索定位货物，Humanoid 负责装卸，UGV 负责运输。",
                "shared_skill_groups": [["T1.0", "T2.0"], ["T3.0", "T4.0", "T5.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "UAV 起飞搜索目标货物", "location": "search_area",
                     "required_skills": [{"skill_name": "search(cargo)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["cargo_location"]},
                    {"task_id": "T2", "description": "UAV 广播货物位置", "location": "search_area",
                     "required_skills": [{"skill_name": "broadcast(cargo_location)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["location_shared"]},
                    {"task_id": "T3", "description": "UGV 导航至货物位置", "location": "tbd:cargo_location",
                     "required_skills": [{"skill_name": "navigate_to(cargo_location)", "assigned_robot_type": ["UGV"], "assigned_robot_count": 1}],
                     "produces": ["ugv_at_cargo"]},
                    {"task_id": "T4", "description": "Humanoid 导航至货物位置", "location": "tbd:cargo_location",
                     "required_skills": [{"skill_name": "navigate_to(cargo_location)", "assigned_robot_type": ["Humanoid"], "assigned_robot_count": 1}],
                     "produces": ["humanoid_at_cargo"]},
                    {"task_id": "T5", "description": "Humanoid 将货物装载到 UGV", "location": "tbd:cargo_location",
                     "required_skills": [{"skill_name": "place(cargo, UGV)", "assigned_robot_type": ["Humanoid"], "assigned_robot_count": 1}],
                     "produces": ["cargo_loaded"]},
                    {"task_id": "T6", "description": "UGV 运输货物到目的地", "location": "destination",
                     "required_skills": [{"skill_name": "navigate_to(destination)", "assigned_robot_type": ["UGV"], "assigned_robot_count": 1}],
                     "produces": ["cargo_delivered"]}
                ],
                "edges": [
                    {"from": "T1", "to": "T2", "type": "sequential"},
                    {"from": "T2", "to": "T3", "type": "sequential"},
                    {"from": "T2", "to": "T4", "type": "parallel"},
                    {"from": "T3", "to": "T5", "type": "sequential"},
                    {"from": "T4", "to": "T5", "type": "sequential"},
                    {"from": "T5", "to": "T6", "type": "sequential"}
                ]
            }
        }
    },
    "search_rescue": {
        "name": "Search and Rescue",
        "description": "搜救任务：多 UAV 协同搜索，RobotDog 救援",
        "data": {
            "meta": {
                "description": "搜救任务，多架 UAV 协同搜索失踪人员，RobotDog 前往救援。",
                "shared_skill_groups": [["T1.0", "T2.0"], ["T3.0", "T4.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "UAV_01 搜索区域 A", "location": "search_zone_A",
                     "required_skills": [{"skill_name": "search(zone_A)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["zone_A_scanned"]},
                    {"task_id": "T2", "description": "UAV_02 搜索区域 B", "location": "search_zone_B",
                     "required_skills": [{"skill_name": "search(zone_B)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["zone_B_scanned"]},
                    {"task_id": "T3", "description": "汇总搜索结果确定目标位置", "location": "command_center",
                     "required_skills": [{"skill_name": "analyze(search_results)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["target_location"]},
                    {"task_id": "T4", "description": "RobotDog 导航至目标位置", "location": "tbd:target_location",
                     "required_skills": [{"skill_name": "navigate_to(target_location)", "assigned_robot_type": ["RobotDog"], "assigned_robot_count": 2}],
                     "produces": ["dogs_at_target"]},
                    {"task_id": "T5", "description": "RobotDog 执行救援操作", "location": "tbd:target_location",
                     "required_skills": [{"skill_name": "rescue(target)", "assigned_robot_type": ["RobotDog"], "assigned_robot_count": 2}],
                     "produces": ["rescue_complete"]}
                ],
                "edges": [
                    {"from": "T1", "to": "T3", "type": "sequential"},
                    {"from": "T2", "to": "T3", "type": "sequential"},
                    {"from": "T3", "to": "T4", "type": "conditional", "condition": "target_location != null"},
                    {"from": "T4", "to": "T5", "type": "sequential"}
                ]
            }
        }
    },
    "perimeter_security": {
        "name": "Perimeter Security",
        "description": "周界安防任务：UAV 高空监控，多 RobotDog 分区巡逻",
        "data": {
            "meta": {
                "description": "周界安防任务，UAV 进行高空全局监控，多个 RobotDog 分区域进行地面巡逻。",
                "shared_skill_groups": [["T1.0"], ["T2.0", "T3.0", "T4.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "UAV 起飞至监控高度", "location": "perimeter_center",
                     "required_skills": [{"skill_name": "navigate_to(altitude_100m)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["uav_monitoring"]},
                    {"task_id": "T2", "description": "RobotDog_01 巡逻北区", "location": "north_sector",
                     "required_skills": [{"skill_name": "patrol(north_route)", "assigned_robot_type": ["RobotDog"], "assigned_robot_count": 1}],
                     "produces": ["north_patrolled"]},
                    {"task_id": "T3", "description": "RobotDog_02 巡逻东区", "location": "east_sector",
                     "required_skills": [{"skill_name": "patrol(east_route)", "assigned_robot_type": ["RobotDog"], "assigned_robot_count": 1}],
                     "produces": ["east_patrolled"]},
                    {"task_id": "T4", "description": "RobotDog_03 巡逻南区", "location": "south_sector",
                     "required_skills": [{"skill_name": "patrol(south_route)", "assigned_robot_type": ["RobotDog"], "assigned_robot_count": 1}],
                     "produces": ["south_patrolled"]},
                    {"task_id": "T5", "description": "汇报巡逻结果", "location": "command_center",
                     "required_skills": [{"skill_name": "report(patrol_status)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["patrol_report"]}
                ],
                "edges": [
                    {"from": "T1", "to": "T2", "type": "parallel"},
                    {"from": "T1", "to": "T3", "type": "parallel"},
                    {"from": "T1", "to": "T4", "type": "parallel"},
                    {"from": "T2", "to": "T5", "type": "sequential"},
                    {"from": "T3", "to": "T5", "type": "sequential"},
                    {"from": "T4", "to": "T5", "type": "sequential"}
                ]
            }
        }
    }
}

# ========== HTML 模板 ==========
HTML_TEMPLATE = '''<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MultiAgent Mock Backend</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1a1a2e; color: #eee; height: 100vh; overflow: hidden;
        }
        .container { display: flex; height: 100vh; }
        
        /* Left Panel - Controls */
        .left-panel {
            width: 380px; background: #16213e; padding: 20px;
            border-right: 1px solid #0f3460; overflow-y: auto;
        }
        .left-panel h2 { color: #e94560; margin-bottom: 20px; font-size: 18px; }
        .hint-box {
            background: #0a3d62; border: 1px solid #00d9ff; border-radius: 8px;
            padding: 12px; margin-bottom: 15px; font-size: 12px; color: #aaa;
        }
        .hint-box .hint-icon { color: #00d9ff; margin-right: 6px; }
        .hint-box strong { color: #00d9ff; }
        .skill-btn {
            width: 100%; padding: 12px 16px; margin-bottom: 10px;
            background: #0f3460; border: 1px solid #e94560; border-radius: 8px;
            color: #fff; cursor: pointer; text-align: left; transition: all 0.2s;
        }
        .skill-btn:hover { background: #e94560; transform: translateX(5px); }
        .skill-btn .name { font-weight: bold; font-size: 14px; }
        .skill-btn .desc { font-size: 12px; color: #aaa; margin-top: 4px; }
        .skill-btn:hover .desc { color: #fff; }
        
        .task-btn {
            width: 100%; padding: 12px 16px; margin-bottom: 10px;
            background: #0f3460; border: 1px solid #00d9ff; border-radius: 8px;
            color: #fff; cursor: pointer; text-align: left; transition: all 0.2s;
        }
        .task-btn:hover { background: #00d9ff; color: #0f3460; transform: translateX(5px); }
        .task-btn .name { font-weight: bold; font-size: 14px; }
        .task-btn .desc { font-size: 12px; color: #aaa; margin-top: 4px; }
        .task-btn:hover .desc { color: #0f3460; }
        
        .section-title { 
            color: #00d9ff; font-size: 14px; margin: 20px 0 10px; 
            padding-bottom: 5px; border-bottom: 1px solid #0f3460;
        }
        .section-title.skill { color: #e94560; }
        .section-title.request-cmd { color: #00ff88; }
        
        .request-cmd-btn {
            width: 100%; padding: 12px 16px; margin-bottom: 10px;
            background: #0f3460; border: 1px solid #00ff88; border-radius: 8px;
            color: #fff; cursor: pointer; text-align: left; transition: all 0.2s;
        }
        .request-cmd-btn:hover { background: #00ff88; color: #0f3460; transform: translateX(5px); }
        .request-cmd-btn .name { font-weight: bold; font-size: 14px; }
        .request-cmd-btn .desc { font-size: 12px; color: #aaa; margin-top: 4px; }
        .request-cmd-btn:hover .desc { color: #0f3460; }
        
        .status { 
            padding: 10px; background: #0a0a1a; border-radius: 8px; 
            margin-top: 20px; font-size: 12px;
        }
        .status .dot { 
            display: inline-block; width: 8px; height: 8px; 
            border-radius: 50%; margin-right: 8px;
        }
        .status .dot.connected { background: #00ff88; }
        .status .dot.disconnected { background: #ff4444; }
        
        /* Right Panel - Messages */
        .right-panel { flex: 1; display: flex; flex-direction: column; }
        .right-header {
            padding: 15px 20px; background: #16213e; 
            border-bottom: 1px solid #0f3460;
            display: flex; justify-content: space-between; align-items: center;
        }
        .right-header h2 { color: #00d9ff; font-size: 16px; }
        .clear-btn {
            padding: 8px 16px; background: #e94560; border: none;
            border-radius: 6px; color: #fff; cursor: pointer; font-size: 12px;
        }
        .clear-btn:hover { background: #ff6b6b; }
        
        .messages {
            flex: 1; overflow-y: auto; padding: 20px;
            background: #0a0a1a;
        }
        .message {
            background: #16213e; border-radius: 8px; padding: 15px;
            margin-bottom: 15px; border-left: 4px solid #00d9ff;
            animation: slideIn 0.3s ease;
        }
        @keyframes slideIn {
            from { opacity: 0; transform: translateX(-20px); }
            to { opacity: 1; transform: translateX(0); }
        }
        /* 仿真 → 后端 (蓝色系) */
        .message.incoming { border-left-color: #00d9ff; background: #0d1f3c; }
        .message.incoming .message-type { background: #0a3d62; color: #00d9ff; }
        .message.incoming .direction { color: #00d9ff; }
        /* 后端 → 仿真 (红色系) */
        .message.outgoing { border-left-color: #e94560; background: #2d1a2e; }
        .message.outgoing .message-type { background: #4a1942; color: #e94560; }
        .message.outgoing .direction { color: #e94560; }
        
        .direction { font-size: 11px; font-weight: bold; margin-bottom: 5px; }
        
        .legend {
            display: flex; gap: 20px; padding: 10px 20px; 
            background: #16213e; border-bottom: 1px solid #0f3460;
            font-size: 12px;
        }
        .legend-item { display: flex; align-items: center; gap: 6px; }
        .legend-dot { width: 12px; height: 12px; border-radius: 3px; }
        .legend-dot.outgoing { background: #e94560; }
        .legend-dot.incoming { background: #00d9ff; }
        
        .message-header {
            display: flex; justify-content: space-between; 
            margin-bottom: 10px; font-size: 12px;
        }
        .message-type { 
            background: #0f3460; padding: 4px 10px; border-radius: 4px;
            color: #00d9ff; font-weight: bold;
        }
        .message-time { color: #666; }
        .message-content {
            background: #0a0a1a; padding: 10px; border-radius: 6px;
            font-family: 'Monaco', 'Consolas', monospace; font-size: 12px;
            white-space: pre-wrap; word-break: break-all; max-height: 300px;
            overflow-y: auto;
        }
        
        .empty-state {
            text-align: center; color: #666; padding: 60px 20px;
        }
        .empty-state svg { width: 80px; height: 80px; margin-bottom: 20px; opacity: 0.3; }
    </style>
</head>
<body>
    <div class="container">
        <div class="left-panel">
            <h2>🎮 Mock Backend Control</h2>
            
            <div class="hint-box">
                <span class="hint-icon">💡</span>
                <span>点击按钮后，数据将发送到 UE5。</span><br>
                <strong>技能列表：请在 UE5 中点击 "Start Executing" 按钮执行。</strong><br>
                <strong>任务图：将显示在任务规划工作台中。</strong>
            </div>
            
            <div class="section-title">📊 预设任务图</div>
            <div id="task-buttons"></div>
            
            <div class="section-title skill">📋 初步技能列表</div>
            <div id="skill-buttons"></div>
            
            <div class="section-title skill">✅ 最终技能列表</div>
            <div id="final-skill-buttons"></div>
            
            <div class="section-title request-cmd">📢 索要用户指令</div>
            <div id="request-cmd-buttons">
                <button class="request-cmd-btn" onclick="sendRequestUserCommand()">
                    <div class="name">📢 一键索要用户指令</div>
                    <div class="desc">请求 UE5 用户输入指令</div>
                </button>
            </div>
            
            <div class="status">
                <span class="dot connected" id="status-dot"></span>
                <span id="status-text">Server Running on :{{PORT}}</span>
            </div>
        </div>
        
        <div class="right-panel">
            <div class="right-header">
                <h2>📨 Communication Log</h2>
                <button class="clear-btn" onclick="clearMessages()">Clear All</button>
            </div>
            <div class="legend">
                <div class="legend-item"><div class="legend-dot outgoing"></div><span>Backend → UE5 (发送)</span></div>
                <div class="legend-item"><div class="legend-dot incoming"></div><span>UE5 → Backend (接收)</span></div>
            </div>
            <div class="messages" id="messages">
                <div class="empty-state" id="empty-state">
                    <svg viewBox="0 0 24 24" fill="currentColor">
                        <path d="M20 2H4c-1.1 0-2 .9-2 2v18l4-4h14c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 14H6l-2 2V4h16v12z"/>
                    </svg>
                    <p>等待 UE5 仿真端连接...</p>
                    <p style="font-size: 12px; margin-top: 10px;">消息将实时显示在这里</p>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        const skillLists = {{SKILL_LISTS}};
        const taskGraphs = {{TASK_GRAPHS}};
        const messagesDiv = document.getElementById('messages');
        const emptyState = document.getElementById('empty-state');
        let messageCount = 0;
        
        // Generate task graph buttons
        const taskButtonsDiv = document.getElementById('task-buttons');
        for (const [key, task] of Object.entries(taskGraphs)) {
            const btn = document.createElement('button');
            btn.className = 'task-btn';
            btn.onclick = () => sendTaskGraph(key);
            btn.innerHTML = `<div class="name">📊 ${task.name}</div><div class="desc">${task.description}</div>`;
            taskButtonsDiv.appendChild(btn);
        }
        
        // Generate skill buttons (初步技能列表)
        const skillButtonsDiv = document.getElementById('skill-buttons');
        for (const [key, skill] of Object.entries(skillLists)) {
            const btn = document.createElement('button');
            btn.className = 'skill-btn';
            btn.onclick = () => sendSkill(key);
            btn.innerHTML = `<div class="name">⚡ ${skill.name}</div><div class="desc">${skill.description}</div>`;
            skillButtonsDiv.appendChild(btn);
        }
        
        // Generate final skill buttons (最终技能列表)
        const finalSkillButtonsDiv = document.getElementById('final-skill-buttons');
        const finalBtn = document.createElement('button');
        finalBtn.className = 'skill-btn';
        finalBtn.style.borderColor = '#00ff88';
        finalBtn.onclick = () => sendFinalSkill();
        finalBtn.innerHTML = `<div class="name">✅ Complete Test (Executable)</div><div class="desc">完整测试 - 可直接执行的最终技能列表</div>`;
        finalSkillButtonsDiv.appendChild(finalBtn);
        
        // Send task graph
        async function sendTaskGraph(taskKey) {
            try {
                const response = await fetch('/api/send_task_graph', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ task_key: taskKey })
                });
                const result = await response.json();
                const displayContent = result.note 
                    ? { ...result, "⚠️ Note": result.note }
                    : result;
                addMessage('sent', `Sent Task Graph: ${taskGraphs[taskKey]?.name || taskKey}`, displayContent, 'outgoing');
            } catch (e) {
                console.error('Failed to send task graph:', e);
            }
        }
        
        // Send skill list (初步技能列表)
        async function sendSkill(skillKey) {
            try {
                const response = await fetch('/api/send_skill', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ skill_key: skillKey })
                });
                const result = await response.json();
                const displayContent = result.note 
                    ? { ...result, "⚠️ Note": result.note }
                    : result;
                addMessage('sent', `Sent Skill List: ${skillLists[skillKey]?.name || skillKey}`, displayContent, 'outgoing');
            } catch (e) {
                console.error('Failed to send skill:', e);
            }
        }
        
        // Send final skill list (最终技能列表 - Executable)
        async function sendFinalSkill() {
            try {
                const response = await fetch('/api/send_final_skill', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({})
                });
                const result = await response.json();
                const displayContent = result.note 
                    ? { ...result, "⚠️ Note": result.note }
                    : result;
                addMessage('sent', `Sent Final Skill List (Executable)`, displayContent, 'outgoing');
            } catch (e) {
                console.error('Failed to send final skill:', e);
            }
        }
        
        // Send request user command (索要用户指令)
        async function sendRequestUserCommand() {
            try {
                const response = await fetch('/api/send_request_user_command', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({})
                });
                const result = await response.json();
                const displayContent = result.note 
                    ? { ...result, "⚠️ Note": result.note }
                    : result;
                addMessage('sent', `Sent Request User Command`, displayContent, 'outgoing');
            } catch (e) {
                console.error('Failed to send request user command:', e);
            }
        }
        
        // Add message to display
        function addMessage(type, title, content, direction) {
            if (emptyState) emptyState.style.display = 'none';
            messageCount++;
            
            const dirClass = direction === 'outgoing' ? 'outgoing' : 'incoming';
            const dirIcon = direction === 'outgoing' ? '📤' : '📥';
            const dirText = direction === 'outgoing' ? 'Backend → UE5' : 'UE5 → Backend';
            
            const msg = document.createElement('div');
            msg.className = `message ${dirClass}`;
            msg.innerHTML = `
                <div class="direction">${dirIcon} ${dirText}</div>
                <div class="message-header">
                    <span class="message-type">${title}</span>
                    <span class="message-time">${new Date().toLocaleTimeString()}</span>
                </div>
                <div class="message-content">${typeof content === 'object' ? JSON.stringify(content, null, 2) : content}</div>
            `;
            messagesDiv.insertBefore(msg, messagesDiv.firstChild);
            
            // Limit messages
            while (messagesDiv.children.length > 100) {
                messagesDiv.removeChild(messagesDiv.lastChild);
            }
        }
        
        function clearMessages() {
            messagesDiv.innerHTML = '';
            emptyState.style.display = 'block';
            messagesDiv.appendChild(emptyState);
        }
        
        // SSE for real-time updates
        const evtSource = new EventSource('/api/messages');
        evtSource.onmessage = (event) => {
            const data = JSON.parse(event.data);
            addMessage(data.type, data.title, data.content, data.direction);
        };
        evtSource.onerror = () => {
            document.getElementById('status-dot').className = 'dot disconnected';
            document.getElementById('status-text').textContent = 'Connection Lost';
        };
    </script>
</body>
</html>'''


# ========== SSE 客户端管理 ==========
sse_clients = []
sse_lock = threading.Lock()

def broadcast_message(msg_type, title, content, direction='incoming'):
    """广播消息给所有 SSE 客户端
    direction: 'incoming' = UE5 → Backend, 'outgoing' = Backend → UE5
    """
    data = json.dumps({
        "type": msg_type, 
        "title": title, 
        "content": content,
        "direction": direction
    }, ensure_ascii=False)
    message_queue.put(data)
    
    # 打印到控制台
    timestamp = datetime.now().strftime('%H:%M:%S')
    arrow = ">>>" if direction == 'outgoing' else "<<<"
    print(f"[{timestamp}] {arrow} {title}")


def create_skill_list_message(skill_list: dict) -> dict:
    """创建技能列表消息"""
    return {
        "message_type": "skill_list",
        "timestamp": int(datetime.now().timestamp() * 1000),
        "message_id": str(uuid.uuid4()),
        "payload": skill_list
    }


def create_task_graph_message(task_graph: dict) -> dict:
    """创建任务图消息"""
    return {
        "message_type": "task_graph",
        "timestamp": int(datetime.now().timestamp() * 1000),
        "message_id": str(uuid.uuid4()),
        "payload": task_graph
    }


def create_request_user_command_message() -> dict:
    """创建索要用户指令消息"""
    return {
        "message_type": "request_user_command",
        "timestamp": int(datetime.now().timestamp() * 1000),
        "message_id": str(uuid.uuid4()),
        "payload": {}
    }


class MockBackendHandler(BaseHTTPRequestHandler):
    """Mock backend request handler with Web UI"""
    
    def log_message(self, format, *args):
        pass
    
    def send_json_response(self, status_code, data):
        self.send_response(status_code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(data, ensure_ascii=False).encode('utf-8'))
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
    
    def do_GET(self):
        parsed_path = urlparse(self.path)
        
        if parsed_path.path == '/' or parsed_path.path == '/index.html':
            self.serve_html()
        elif parsed_path.path == '/api/sim/poll':
            self.handle_poll()
        elif parsed_path.path == '/api/messages':
            self.handle_sse()
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        parsed_path = urlparse(self.path)
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8') if content_length > 0 else ''
        
        if parsed_path.path == '/api/sim/message':
            self.handle_message(body)
        elif parsed_path.path == '/api/sim/scene_change':
            self.handle_scene_change(body)
        elif parsed_path.path == '/api/send_skill':
            self.handle_send_skill(body)
        elif parsed_path.path == '/api/send_final_skill':
            self.handle_send_final_skill(body)
        elif parsed_path.path == '/api/send_task_graph':
            self.handle_send_task_graph(body)
        elif parsed_path.path == '/api/send_request_user_command':
            self.handle_send_request_user_command()
        else:
            self.send_response(404)
            self.end_headers()
    
    def serve_html(self):
        """Serve the Web UI"""
        global server_port
        html = HTML_TEMPLATE.replace('{{PORT}}', str(server_port))
        html = html.replace('{{SKILL_LISTS}}', json.dumps(SKILL_LISTS, ensure_ascii=False))
        html = html.replace('{{TASK_GRAPHS}}', json.dumps(TASK_GRAPHS, ensure_ascii=False))
        
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(html.encode('utf-8'))
    
    def handle_poll(self):
        """Handle UE5 poll request"""
        global pending_messages
        with message_lock:
            if pending_messages:
                msg = pending_messages.pop(0)
                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps([msg], ensure_ascii=False).encode('utf-8'))
                msg_type = msg.get('message_type', 'unknown')
                broadcast_message('sent', msg_type, msg, 'outgoing')
            else:
                self.send_response(204)
                self.end_headers()
    
    def handle_sse(self):
        """Handle SSE connection for real-time updates"""
        self.send_response(200)
        self.send_header('Content-Type', 'text/event-stream')
        self.send_header('Cache-Control', 'no-cache')
        self.send_header('Connection', 'keep-alive')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        
        try:
            while True:
                try:
                    data = message_queue.get(timeout=30)
                    self.wfile.write(f"data: {data}\n\n".encode('utf-8'))
                    self.wfile.flush()
                except queue.Empty:
                    # Send heartbeat
                    self.wfile.write(": heartbeat\n\n".encode('utf-8'))
                    self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            pass
    
    def handle_message(self, body):
        """Handle message from UE5"""
        try:
            data = json.loads(body)
            msg_type = data.get('message_type', 'unknown')
            
            broadcast_message('message', msg_type, data, 'incoming')
            self.send_json_response(200, {"status": "received"})
            
        except json.JSONDecodeError as e:
            broadcast_message('error', 'JSON Parse Error', str(e), 'incoming')
            self.send_json_response(400, {"error": str(e)})
    
    def handle_scene_change(self, body):
        """Handle scene change from UE5"""
        try:
            data = json.loads(body)
            change_type = data.get('change_type', 'unknown')
            broadcast_message('scene_change', f'scene_change: {change_type}', data, 'incoming')
            self.send_json_response(200, {"status": "received"})
        except json.JSONDecodeError as e:
            self.send_json_response(400, {"error": str(e)})
    
    def handle_send_skill(self, body):
        """Handle skill send request from Web UI (初步技能列表)
        
        Note: This method only queues the skill list message for UE5 to poll.
        The skill list will be saved to skill_list_temp.json by UE5's TempDataManager.
        Users must manually click "Start Executing" in UE5 to execute the skills.
        This method does NOT directly trigger skill execution.
        """
        global pending_messages
        try:
            data = json.loads(body)
            skill_key = data.get('skill_key', '')
            
            if skill_key in SKILL_LISTS:
                skill_data = SKILL_LISTS[skill_key]['data']
                msg = create_skill_list_message(skill_data)
                
                with message_lock:
                    pending_messages.append(msg)
                
                self.send_json_response(200, {
                    "status": "queued",
                    "skill": skill_key,
                    "message_id": msg['message_id'],
                    "note": "Skill list queued. Click 'Start Executing' in UE5 to run."
                })
            else:
                self.send_json_response(400, {"error": f"Unknown skill: {skill_key}"})
                
        except json.JSONDecodeError as e:
            self.send_json_response(400, {"error": str(e)})
    
    def handle_send_final_skill(self, body):
        """Handle final skill send request from Web UI (最终技能列表 - Executable)
        
        This sends the Complete Test skill list with an additional 'executable' field
        set to true, indicating this is the final executable skill list.
        """
        global pending_messages
        try:
            # Use Complete Test skill data
            skill_data = SKILL_LISTS['complete_test']['data']
            msg = create_skill_list_message(skill_data)
            # Add executable field to indicate this is the final executable skill list
            msg['executable'] = True
            
            with message_lock:
                pending_messages.append(msg)
            
            self.send_json_response(200, {
                "status": "queued",
                "skill": "complete_test (executable)",
                "message_id": msg['message_id'],
                "executable": True,
                "note": "Final executable skill list queued."
            })
                
        except json.JSONDecodeError as e:
            self.send_json_response(400, {"error": str(e)})
    
    def handle_send_task_graph(self, body):
        """Handle task graph send request from Web UI
        
        Note: This method queues the task graph message for UE5 to poll.
        The task graph will be saved to task_graph_temp.json by UE5's TempDataManager.
        The task graph will be displayed in the Task Planner Widget.
        """
        global pending_messages
        try:
            data = json.loads(body)
            task_key = data.get('task_key', '')
            
            if task_key in TASK_GRAPHS:
                task_data = TASK_GRAPHS[task_key]['data']
                msg = create_task_graph_message(task_data)
                
                with message_lock:
                    pending_messages.append(msg)
                
                self.send_json_response(200, {
                    "status": "queued",
                    "task_graph": task_key,
                    "message_id": msg['message_id'],
                    "note": "Task graph queued. It will be displayed in Task Planner Widget."
                })
            else:
                self.send_json_response(400, {"error": f"Unknown task graph: {task_key}"})
                
        except json.JSONDecodeError as e:
            self.send_json_response(400, {"error": str(e)})

    def handle_send_request_user_command(self):
        """Handle request user command send request from Web UI
        
        Note: This method queues the request_user_command message for UE5 to poll.
        UE5 will display a notification prompting the user to enter a command.
        """
        global pending_messages
        msg = create_request_user_command_message()
        
        with message_lock:
            pending_messages.append(msg)
        
        # Broadcast to communication log
        broadcast_message('sent', 'request_user_command', msg, 'outgoing')
        
        self.send_json_response(200, {
            "status": "queued",
            "message_type": "request_user_command",
            "message_id": msg['message_id'],
            "note": "Request user command message queued. UE5 will display a notification."
        })


server_port = 8080


def main():
    global server_port
    
    parser = argparse.ArgumentParser(description='MultiAgent-Unreal Mock Backend Server with Web UI')
    parser.add_argument('--port', type=int, default=8080, help='Server port (default: 8080)')
    parser.add_argument('--host', type=str, default='0.0.0.0', help='Server address (default: 0.0.0.0)')
    args = parser.parse_args()
    
    server_port = args.port
    server_address = (args.host, args.port)
    
    # Use ThreadingHTTPServer for concurrent connections
    from http.server import ThreadingHTTPServer
    httpd = ThreadingHTTPServer(server_address, MockBackendHandler)
    
    print("=" * 60)
    print("  MultiAgent-Unreal Mock Backend Server")
    print("=" * 60)
    print(f"  🌐 Web UI:  http://localhost:{args.port}")
    print(f"  📡 API:     http://{args.host}:{args.port}/api/sim/poll")
    print("=" * 60)
    print(f"  Waiting for connections...")
    print(f"  Press Ctrl+C to stop")
    print("=" * 60)
    print()
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped")
        httpd.shutdown()
        sys.exit(0)


if __name__ == '__main__':
    main()
