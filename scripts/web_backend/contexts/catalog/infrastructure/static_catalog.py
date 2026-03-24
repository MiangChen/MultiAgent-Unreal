from __future__ import annotations

from dataclasses import dataclass

# Static demo/mock data used by the web backend.

SKILL_ALLOCATIONS = {
    "complete_test": {
        "name": "Complete Test",
        "description": "Complete test: UAV search -> navigate load -> transport unload",
        "data": {
            "0": {
                "UAV-1": {"skill": "take_off", "params": {}},
                "UAV-2": {"skill": "take_off", "params": {}}
            },
            "1": {
                "UAV-1": {"skill": "search", "params": {
                    "search_area": [[-500, 1000], [1500, 1000], [1500, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }},
                "UAV-2": {"skill": "search", "params": {
                    "search_area": [[-1000, 1000], [-2000, 1000], [-2000, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }}
            },
            "2": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": "redbox_x + 100", "y": "redbox_y - 400", "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": "redbox_x - 100", "y": "redbox_y - 200", "z": 0}}},
                "UAV-1": {"skill": "return_home", "params": {}},
                "UAV-2": {"skill": "return_home", "params": {}}
            },
            "3": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "robot", "type": "UGV", "features": {"label": "UGV-1"}}
                }}
            },
            "4": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": -5100, "y": 2200, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -5200, "y": 2100, "z": 0}}}
            },
            "5": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "ground", "type": "", "features": {}}
                }}
            }
        }
    },
    "uav_search": {
        "name": "UAV Cooperative Search",
        "description": "2 UAVs cooperative search for RedBox",
        "data": {
            "0": {
                "UAV-1": {"skill": "take_off", "params": {}},
                "UAV-2": {"skill": "take_off", "params": {}}
            },
            "1": {
                "UAV-1": {"skill": "search", "params": {
                    "search_area": [[-500, 1000], [1500, 1000], [1500, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }},
                "UAV-2": {"skill": "search", "params": {
                    "search_area": [[-1000, 1000], [-2000, 1000], [-2000, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }}
            }
        }
    },
    "fire_rescue": {
        "name": "Fire Rescue Execution (Gantt)",
        "description": "Skill allocation result for fire rescue",
        "data": {
            "0": {
                "UAV-1": {"skill": "take_off", "params": {}},
                "Quadruped-1": {"skill": "navigate", "params": {"dest": {"x": 1000, "y": 2000, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -1000, "y": 2000, "z": 0}}}
            },
            "1": {
                "UAV-1": {"skill": "search", "params": {
                    "search_area": [[0, 0], [2000, 0], [2000, 2000], [0, 2000]],
                    "target": {"class": "event", "type": "fire", "features": {}}
                }}
            }
        }
    },
    "navigate_load": {
        "name": "Navigate to RedBox and Load to UGV",
        "description": "UGV and Humanoid navigate to RedBox, then Humanoid loads RedBox onto UGV",
        "data": {
            "0": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": "redbox_x + 100", "y": "redbox_y - 400", "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": "redbox_x - 100", "y": "redbox_y - 200", "z": 0}}},
                "UAV-1": {"skill": "return_home", "params": {}},
                "UAV-2": {"skill": "return_home", "params": {}}
            },
            "1": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "robot", "type": "UGV", "features": {"label": "UGV-1"}}
                }}
            }
        }
    },
    "transport_unload": {
        "name": "Transport to Destination and Unload",
        "description": "UGV and Humanoid navigate to the destination, then Humanoid unloads RedBox from UGV to the ground",
        "data": {
            "0": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": -5100, "y": 2200, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -5200, "y": 2100, "z": 0}}}
            },
            "1": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "ground", "type": "", "features": {}}
                }}
            }
        }
    },
    "single": {
        "name": "Single Test",
        "description": "UAV takeoff -> navigate -> Humanoid navigate",
        "data": {
            "0": {"UAV-1": {"skill": "take_off", "params": {}}},
            "1": {"UAV-1": {"skill": "navigate", "params": {"dest": {"x": 1350, "y": -450, "z": 545}}}},
            "2": {"Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -1200, "y": 1200, "z": 0}}}},
            "3": {"Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": 0, "y": 2400, "z": 0}}}}
        }
    },
    "place_coop": {
        "name": "Place Coop",
        "description": "UGV + Humanoid cooperative transport",
        "data": {
            "0": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": 400, "y": 1300, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": 200, "y": 1850, "z": 0}}}
            },
            "1": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "robot", "type": "UGV", "features": {"label": "UGV-1"}}
                }}
            },
            "2": {"UGV-1": {"skill": "navigate", "params": {"dest": {"x": 1000, "y": 1000, "z": 0}}}},
            "3": {"Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": 1150, "y": 1000, "z": 0}}}},
            "4": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "ground", "type": "", "features": {}}
                }}
            }
        }
    }
}

# ========== 技能列表定义 (Skill List - 直接执行) ==========
# 用于直接执行，无需 UI 交互

SKILL_LISTS = {
    "complete_test": {
        "name": "Complete Test",
        "description": "Complete test: UAV search -> navigate load -> transport unload",
        "data": {
            "0": {
                "UAV-1": {"skill": "take_off", "params": {}},
                "UAV-2": {"skill": "take_off", "params": {}}
            },
            "1": {
                "UAV-1": {"skill": "search", "params": {
                    "search_area": [[-2000, 6000], [-5000, 6000], [-5000, 9000], [-2000, 9000]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }},
                "UAV-2": {"skill": "search", "params": {
                    "search_area": [[-2000, 9000], [-5000, 9000], [-5000, 12000], [-2000, 12000]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }}
            },
            "2": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": -2300, "y": 9000, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -2400, "y": 9100, "z": 0}}},
                "UAV-1": {"skill": "return_home", "params": {}},
                "UAV-2": {"skill": "return_home", "params": {}}
            },
            "3": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "robot", "type": "UGV", "features": {"label": "UGV-1"}}
                }}
            },
            "4": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": -8100, "y": 9200, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -8200, "y": 9500, "z": 0}}}
            },
            "5": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "ground", "type": "", "features": {}}
                }}
            }
        }
    },
    "uav_search": {
        "name": "UAV Cooperative Search",
        "description": "2 UAVs cooperative search for RedBox",
        "data": {
            "0": {
                "UAV-1": {"skill": "take_off", "params": {}},
                "UAV-2": {"skill": "take_off", "params": {}}
            },
            "1": {
                "UAV-1": {"skill": "search", "params": {
                    "search_area": [[-500, 1000], [1500, 1000], [1500, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }},
                "UAV-2": {"skill": "search", "params": {
                    "search_area": [[-1000, 1000], [-2000, 1000], [-2000, 2500], [-500, 2500]],
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}}
                }}
            }
        }
    },
    "fire_rescue": {
        "name": "Fire Rescue Execution",
        "description": "Direct execution commands for fire rescue",
        "data": {
            "0": {
                "UAV-1": {"skill": "take_off", "params": {}},
                "Quadruped-1": {"skill": "navigate", "params": {"dest": {"x": 1000, "y": 2000, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -1000, "y": 2000, "z": 0}}}
            },
            "1": {
                "UAV-1": {"skill": "search", "params": {
                    "search_area": [[0, 0], [2000, 0], [2000, 2000], [0, 2000]],
                    "target": {"class": "event", "type": "fire", "features": {}}
                }}
            }
        }
    },
    "navigate_load": {
        "name": "Navigate to RedBox and Load to UGV",
        "description": "UGV and Humanoid navigate to RedBox, then Humanoid loads RedBox onto UGV",
        "data": {
            "0": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": "redbox_x + 100", "y": "redbox_y - 400", "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": "redbox_x - 100", "y": "redbox_y - 200", "z": 0}}},
                "UAV-1": {"skill": "return_home", "params": {}},
                "UAV-2": {"skill": "return_home", "params": {}}
            },
            "1": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "robot", "type": "UGV", "features": {"label": "UGV-1"}}
                }}
            }
        }
    },
    "transport_unload": {
        "name": "Transport to Destination and Unload",
        "description": "UGV and Humanoid navigate to the destination, then Humanoid unloads RedBox from UGV to the ground",
        "data": {
            "0": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": -5100, "y": 2200, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -5200, "y": 2100, "z": 0}}}
            },
            "1": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "ground", "type": "", "features": {}}
                }}
            }
        }
    },
    "target_following": {
        "name": "Target Following",
        "description": "UAV takes off and follows the red sedan",
        "data": {
            "0": {'UAV-1': {'skill': 'take_off', 'params': {'task_id': 'T1'}}},
            "1": {'UAV-1': {'skill': 'follow', 'params': {'target': {'class': 'object', 'type': 'vehicle', 'features': {'color': 'red', 'subtype': 'sedan'}}, 'target_token': 'red_sedan', 'task_id': 'T2'}}}
        }
    },
    "guidance_vehicle": {
        "name": "Vehicle Guidance",
        "description": "Quadruped guides the silver hatchback to the designated intersection",
        "data": {
            "0": {'Quadruped-1': {'skill': 'navigate', 'params': {'area_token': 'Car_5', 'object_id': '4009', 'dest_id': '4009', 'dest': {'x': 5200.0, 'y': 4400.0, 'z': 0.0}, 'task_id': 'T2'}}},
            "1": {'Quadruped-1': {'skill': 'guide', 'params': {'target': {'token': 'silver_hatchback', 'class': 'object', 'type': 'vehicle', 'features': {'subtype': 'hatchback', 'color': 'silver'}, 'conf_ge': 0.85, 'persist_ge_s': 1.0}, 'target_token': 'silver_hatchback', 'object_id': '4009', 'dest_token': 'Intersection-4', 'dest': {'x': -6703.0, 'y': 12800.0, 'z': 0.0}, 'task_id': 'T2'}}}
        }
    },
    "guidance_person": {
        "name": "Person Guidance",
        "description": "Quadruped guides the pedestrian to the designated intersection",
        "data": {
            "0": {'Quadruped-1': {'skill': 'navigate', 'params': {'area_token': 'Person_5', 'object_id': '3004', 'dest_id': '3004', 'dest': {'x': -7000.0, 'y': 3900.0, 'z': 0.0}, 'task_id': 'T2'}}},
            "1": {'Quadruped-1': {'skill': 'guide', 'params': {'target': {'token': 'sportive_male', 'class': 'object', 'type': 'person', 'features': {'subtype': 'sportive', 'gender': 'male', 'clothing_color': 'brown'}, 'conf_ge': 0.85, 'persist_ge_s': 1.0}, 'target_token': 'sportive_male', 'object_id': '3004', 'dest_token': 'Intersection-4', 'dest': {'x': -6703.0, 'y': 12800.0, 'z': 0.0}, 'task_id': 'T2'}}}
        }
    },
    "traffic_enforcement_broadcast": {
        "name": "Traffic Enforcement (Broadcast)",
        "description": "UAV broadcasts a warning to the illegally parked vehicle",
        "data": {
            "0": {'UAV-1': {'skill': 'take_off', 'params': {'task_id': 'T2'}}},
            "1": {'UAV-1': {'skill': 'navigate', 'params': {'area_token': 'Vehicle-27', 'object_id': '102', 'dest_id': '102', 'dest': {'x': -8586.5419921875, 'y': 3313.423583984375, 'z': 0.0}, 'task_id': 'T2'}}},
            "2": {'UAV-1': {'skill': 'broadcast', 'params': {'target': {'class': 'event', 'event_type': 'traffic_violation', 'conf_ge': 0.85, 'persist_ge_s': 2.0}, 'target_token': 'traffic_violation', 'conf_ge': 0.85, 'persist_ge_s': 2.0, 'message': 'No parking here, please leave immediately.', 'duration_ge_s': 5.0, 'object_id': '102', 'task_id': 'T3'}}}
        }
    },
    "traffic_enforcement_photo": {
        "name": "Traffic Enforcement (Photo)",
        "description": "UAV photographs the illegally parked vehicle for evidence",
        "data": {
            "0": {'UAV-1': {'skill': 'take_off', 'params': {'task_id': 'T2'}}},
            "1": {'UAV-1': {'skill': 'navigate', 'params': {'area_token': 'Vehicle-27', 'object_id': '102', 'dest_id': '102', 'dest': {'x': -8586.5419921875, 'y': 3313.423583984375, 'z': 0.0}, 'task_id': 'T2'}}},
            "2": {'UAV-1': {'skill': 'take_photo', 'params': {'target': {'class': 'event', 'event_type': 'traffic_violation', 'conf_ge': 0.85, 'persist_ge_s': 2.0}, 'target_token': 'traffic_violation', 'bind_event_type': 'traffic_violation', 'object_id': '102', 'task_id': 'T3'}}}
        }
    },
    "assembly": {
        "name": "Assembly Task",
        "description": "UGV + Humanoid cooperative antenna module assembly",
        "data": {
            "0": {'UGV-1': {'skill': 'navigate', 'params': {'area_token': 'Streetlight-59', 'object_id': '59', 'dest': {'x': -4513.3408203125, 'y': 1819.456787109375, 'z': 0.0}, 'task_id': 'T1'}}, 'Humanoid-1': {'skill': 'navigate', 'params': {'area_token': 'Streetlight-59', 'object_id': '59', 'dest': {'x': -4513.3408203125, 'y': 1819.456787109375, 'z': 0.0}, 'task_id': 'T1'}}},
            "1": {'Humanoid-1': {'skill': 'place', 'params': {'target': {'class': 'object', 'type': 'assembly_component', 'features': {'subtype': 'stand'}}, 'target_token': 'stand', 'object_id': '9003', 'surface_token': 'UGV', 'surface_target': {'class': 'robot', 'type': 'UGV', 'features': {'label': 'UGV-1'}}, 'task_id': 'T1'}}},
            "2": {'UGV-1': {'skill': 'navigate', 'params': {'area_token': 'Streetlight-45', 'object_id': '45', 'dest': {'x': -5438.4375, 'y': 11152.955078125, 'z': 0.0}, 'task_id': 'T1'}}, 'Humanoid-1': {'skill': 'navigate', 'params': {'area_token': 'Streetlight-45', 'object_id': '45', 'dest': {'x': -5438.4375, 'y': 11152.955078125, 'z': 0.0}, 'task_id': 'T1'}}},
            "3": {'Humanoid-1': {'skill': 'place', 'params': {'target': {'class': 'object', 'type': 'assembly_component', 'features': {'subtype': 'stand'}}, 'target_token': 'stand', 'object_id': '9003', 'surface_token': 'ground', 'surface_target': {'class': 'ground', 'type': '', 'features': {}}, 'task_id': 'T1'}}},
            "4": {'UGV-1': {'skill': 'navigate', 'params': {'area_token': 'Streetlight-59', 'object_id': '59', 'dest': {'x': -4513.3408203125, 'y': 1819.456787109375, 'z': 0.0}, 'task_id': 'T2'}}, 'Humanoid-1': {'skill': 'navigate', 'params': {'area_token': 'Streetlight-59', 'object_id': '59', 'dest': {'x': -4513.3408203125, 'y': 1819.456787109375, 'z': 0.0}, 'task_id': 'T2'}}},
            "5": {'Humanoid-1': {'skill': 'place', 'params': {'target': {'class': 'object', 'type': 'assembly_component', 'features': {'subtype': 'antenna_module'}}, 'target_token': 'antenna_module', 'object_id': '9001', 'surface_token': 'UGV', 'surface_target': {'class': 'robot', 'type': 'UGV', 'features': {'label': 'UGV-1'}}, 'task_id': 'T2'}}},
            "6": {'UGV-1': {'skill': 'navigate', 'params': {'area_token': 'Streetlight-45', 'object_id': '45', 'dest': {'x': -5438.4375, 'y': 11152.955078125, 'z': 0.0}, 'task_id': 'T2'}}, 'Humanoid-1': {'skill': 'navigate', 'params': {'area_token': 'Streetlight-45', 'object_id': '45', 'dest': {'x': -5438.4375, 'y': 11152.955078125, 'z': 0.0}, 'task_id': 'T2'}}},
            "7": {'Humanoid-1': {'skill': 'place', 'params': {'target': {'class': 'object', 'type': 'assembly_component', 'features': {'subtype': 'antenna_module'}}, 'target_token': 'antenna_module', 'object_id': '9001', 'surface_token': 'ground', 'surface_target': {'class': 'ground', 'type': '', 'features': {}}, 'task_id': 'T2'}}},
            "8": {'Humanoid-1': {'skill': 'place', 'params': {'target': {'class': 'object', 'type': 'assembly_component', 'features': {'subtype': 'antenna_module'}}, 'target_token': 'antenna_module', 'object_id': '9001', 'surface_token': 'stand', 'surface_target': {'class': 'object', 'type': 'assembly_component', 'features': {'subtype': 'stand'}}, 'surface_id': '9003', 'task_id': 'T2'}}}
        }
    },
    "place_coop": {
        "name": "Place Coop",
        "description": "UGV + Humanoid cooperative transport",
        "data": {
            "0": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": -2900, "y": 9000, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -2400, "y": 9300, "z": 0}}},
            },
            "1": {
                "Humanoid-1": {"skill": "place", "params": {
                    "target": {"class": "object", "type": "cargo", "features": {"color": "red", "label": "RedBox"}},
                    "surface_target": {"class": "robot", "type": "UGV", "features": {"label": "UGV-1"}}
                }}
            },
            "2": {
                "UGV-1": {"skill": "navigate", "params": {"dest": {"x": -8100, "y": 12200, "z": 0}}},
                "Humanoid-1": {"skill": "navigate", "params": {"dest": {"x": -8200, "y": 12700, "z": 0}}}
            },
            "3": {
                "Humanoid-1": {"skill": "place", "params": {
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
        "description": "Deploy UAV to scout the fire area, send Quadruped units to suppress the fire, and send a Humanoid to evacuate personnel",
        "data": {
            "meta": {
                "description": "Deploy UAV to scout the fire area, send Quadruped units to suppress the fire, and send a Humanoid to evacuate personnel.",
                "shared_skill_groups": [["T1.0", "T4.0"], ["T2.0", "T5.0"], ["T3.0", "T6.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "UAV flies to the fire area for aerial reconnaissance", "location": "building_A_roof",
                     "required_skills": [{"skill_name": "navigate_to(building_A_roof)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["drone_at_fire_zone"]},
                    {"task_id": "T2", "description": "UAV scans the fire area for thermal imaging data", "location": "building_A_roof",
                     "required_skills": [{"skill_name": "perceive(thermal_scan)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["fire_location_data", "victim_locations"]},
                    {"task_id": "T3", "description": "Quadruped navigates to the fire scene entrance", "location": "building_A_entrance",
                     "required_skills": [{"skill_name": "navigate_to(building_A_entrance)", "assigned_robot_type": ["Quadruped"], "assigned_robot_count": 2}],
                     "produces": ["dogs_at_entrance"]},
                    {"task_id": "T4", "description": "UAV broadcasts the fire location to ground robots", "location": "building_A_roof",
                     "required_skills": [{"skill_name": "broadcast(fire_location_data)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["fire_info_shared"]},
                    {"task_id": "T5", "description": "Quadruped performs fire suppression", "location": "tbd:fire_point",
                     "required_skills": [{"skill_name": "manipulate(fire_extinguisher)", "assigned_robot_type": ["Quadruped"], "assigned_robot_count": 2}],
                     "produces": ["fire_suppressed"]},
                    {"task_id": "T6", "description": "Humanoid evacuates trapped personnel", "location": "tbd:victim_location",
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
        "description": "Warehouse patrol: UAV aerial surveillance, Quadruped ground patrol",
        "data": {
            "meta": {
                "description": "Warehouse security patrol with UAV aerial surveillance and Quadruped ground inspection.",
                "shared_skill_groups": [["T1.0", "T2.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "UAV takes off and flies to the warehouse airspace", "location": "warehouse_center",
                     "required_skills": [{"skill_name": "navigate_to(warehouse_center)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["uav_at_position"]},
                    {"task_id": "T2", "description": "UAV performs area surveillance scan", "location": "warehouse_center",
                     "required_skills": [{"skill_name": "perceive(area_scan)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["surveillance_data"]},
                    {"task_id": "T3", "description": "Quadruped navigates to the warehouse entrance", "location": "warehouse_entrance",
                     "required_skills": [{"skill_name": "navigate_to(warehouse_entrance)", "assigned_robot_type": ["Quadruped"], "assigned_robot_count": 1}],
                     "produces": ["dog_at_entrance"]},
                    {"task_id": "T4", "description": "Quadruped performs ground patrol", "location": "warehouse_interior",
                     "required_skills": [{"skill_name": "patrol(warehouse_route)", "assigned_robot_type": ["Quadruped"], "assigned_robot_count": 1}],
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
        "description": "Cargo delivery: UAV searches for cargo, Humanoid loads it, and UGV transports it",
        "data": {
            "meta": {
                "description": "Cooperative cargo delivery: UAV locates cargo, Humanoid handles loading and unloading, and UGV transports it.",
                "shared_skill_groups": [["T1.0", "T2.0"], ["T3.0", "T4.0", "T5.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "UAV takes off to search for the target cargo", "location": "search_area",
                     "required_skills": [{"skill_name": "search(cargo)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["cargo_location"]},
                    {"task_id": "T2", "description": "UAV broadcasts the cargo location", "location": "search_area",
                     "required_skills": [{"skill_name": "broadcast(cargo_location)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["location_shared"]},
                    {"task_id": "T3", "description": "UGV navigates to the cargo location", "location": "tbd:cargo_location",
                     "required_skills": [{"skill_name": "navigate_to(cargo_location)", "assigned_robot_type": ["UGV"], "assigned_robot_count": 1}],
                     "produces": ["ugv_at_cargo"]},
                    {"task_id": "T4", "description": "Humanoid navigates to the cargo location", "location": "tbd:cargo_location",
                     "required_skills": [{"skill_name": "navigate_to(cargo_location)", "assigned_robot_type": ["Humanoid"], "assigned_robot_count": 1}],
                     "produces": ["humanoid_at_cargo"]},
                    {"task_id": "T5", "description": "Humanoid loads the cargo onto the UGV", "location": "tbd:cargo_location",
                     "required_skills": [{"skill_name": "place(cargo, UGV)", "assigned_robot_type": ["Humanoid"], "assigned_robot_count": 1}],
                     "produces": ["cargo_loaded"]},
                    {"task_id": "T6", "description": "UGV transports the cargo to the destination", "location": "destination",
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
        "description": "Search and rescue: multiple UAVs cooperate in search, then Quadruped performs the rescue",
        "data": {
            "meta": {
                "description": "Search and rescue mission: multiple UAVs cooperate to search for the missing person, then Quadruped moves in for rescue.",
                "shared_skill_groups": [["T1.0", "T2.0"], ["T3.0", "T4.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "UAV-1 searches zone A", "location": "search_zone_A",
                     "required_skills": [{"skill_name": "search(zone_A)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["zone_A_scanned"]},
                    {"task_id": "T2", "description": "UAV-2 searches zone B", "location": "search_zone_B",
                     "required_skills": [{"skill_name": "search(zone_B)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["zone_B_scanned"]},
                    {"task_id": "T3", "description": "Aggregate search results to identify the target location", "location": "command_center",
                     "required_skills": [{"skill_name": "analyze(search_results)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["target_location"]},
                    {"task_id": "T4", "description": "Quadruped navigates to the target location", "location": "tbd:target_location",
                     "required_skills": [{"skill_name": "navigate_to(target_location)", "assigned_robot_type": ["Quadruped"], "assigned_robot_count": 2}],
                     "produces": ["dogs_at_target"]},
                    {"task_id": "T5", "description": "Quadruped performs the rescue action", "location": "tbd:target_location",
                     "required_skills": [{"skill_name": "rescue(target)", "assigned_robot_type": ["Quadruped"], "assigned_robot_count": 2}],
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
        "description": "Perimeter security: UAV high-altitude surveillance with multiple Quadrupeds patrolling separate sectors",
        "data": {
            "meta": {
                "description": "Perimeter security mission with UAV global surveillance and multiple Quadrupeds patrolling designated sectors.",
                "shared_skill_groups": [["T1.0"], ["T2.0", "T3.0", "T4.0"]]
            },
            "task_graph": {
                "nodes": [
                    {"task_id": "T1", "description": "UAV climbs to surveillance altitude", "location": "perimeter_center",
                     "required_skills": [{"skill_name": "navigate_to(altitude_100m)", "assigned_robot_type": ["UAV"], "assigned_robot_count": 1}],
                     "produces": ["uav_monitoring"]},
                    {"task_id": "T2", "description": "Quadruped_01 patrols the north sector", "location": "north_sector",
                     "required_skills": [{"skill_name": "patrol(north_route)", "assigned_robot_type": ["Quadruped"], "assigned_robot_count": 1}],
                     "produces": ["north_patrolled"]},
                    {"task_id": "T3", "description": "Quadruped_02 patrols the east sector", "location": "east_sector",
                     "required_skills": [{"skill_name": "patrol(east_route)", "assigned_robot_type": ["Quadruped"], "assigned_robot_count": 1}],
                     "produces": ["east_patrolled"]},
                    {"task_id": "T4", "description": "Quadruped_03 patrols the south sector", "location": "south_sector",
                     "required_skills": [{"skill_name": "patrol(south_route)", "assigned_robot_type": ["Quadruped"], "assigned_robot_count": 1}],
                     "produces": ["south_patrolled"]},
                    {"task_id": "T5", "description": "Report patrol results", "location": "command_center",
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


@dataclass(slots=True)
class StaticCatalog:
    skill_allocations: dict[str, dict]
    skill_lists: dict[str, dict]
    task_graphs: dict[str, dict]

    def get_skill_allocation(self, allocation_key: str) -> dict | None:
        return self.skill_allocations.get(allocation_key)

    def get_skill_list(self, skill_key: str) -> dict | None:
        return self.skill_lists.get(skill_key)

    def get_task_graph(self, task_key: str) -> dict | None:
        return self.task_graphs.get(task_key)


def build_static_catalog() -> StaticCatalog:
    return StaticCatalog(
        skill_allocations=SKILL_ALLOCATIONS,
        skill_lists=SKILL_LISTS,
        task_graphs=TASK_GRAPHS,
    )
