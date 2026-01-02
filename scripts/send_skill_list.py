#!/usr/bin/env python3
"""
发送技能列表到 UE5 仿真端并接收执行反馈

使用方法:
    # 测试单个技能列表
    python scripts/send_skill_list.py --server --test single
    
    # 测试 UAV 协同搜索
    python scripts/send_skill_list.py --server --test uav_search
    
    # 测试 UGV+Humanoid 搬运（依据反馈自动发送）
    python scripts/send_skill_list.py --server --test transport_auto
    
    # 测试 Place 技能 - UGV 和 Humanoid 协作搬运
    python scripts/send_skill_list.py --server --test place_coop
"""

import json
import argparse
import threading
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime
import uuid

# ========== 配置 ==========
# 从 environment.json 读取的默认 RedBox 位置
DEFAULT_REDBOX_POSITION = {"x": 300, "y": 2400, "z": 0}

# ========== 测试技能列表定义 ==========

# 单个技能列表：UAV 起飞 -> 导航 -> Humanoid 导航两次
SKILL_LIST_SINGLE = {
    "0": {
        "UAV_01": {"skill": "take_off", "params": {}}
    },
    "1": {
        "UAV_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 1350, "y": -450, "z": 545}, "target_entity": "SM_FireHydrant"}
        }
    },
    "2": {
        "Humanoid_01": {
            "skill": "navigate",
            "params": {"dest": {"x": -1200, "y": 1200, "z": 0}}
        }
    },
    "3": {
        "Humanoid_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 0, "y": 2400, "z": 0}}
        }
    }
}

# ========== UAV 协同搜索技能列表 ==========
# 2个 UAV 同时执行搜索技能，边界范围组合起来是一个完整的区域
# 搜索目标: RedBox (id: 1001)

SKILL_LIST_UAV_SEARCH = {
    "0": {
        # UAV_01 和 UAV_02 同时起飞
        "UAV_01": {"skill": "take_off", "params": {}},
        "UAV_02": {"skill": "take_off", "params": {}}
    },
    "1": {
        # UAV_01 搜索左半区域 (x: -1000 ~ 500)
        "UAV_01": {
            "skill": "search",
            "params": {
                "search_area": [
                    [-500, 1000],
                    [1500, 1000],
                    [1500, 2500],
                    [-500, 2500]
                ],
                "target": {
                    "class": "object",
                    "type": "box",
                    "features": {"color": "red", "label": "RedBox"}
                }
            }
        },
        # UAV_02 搜索右半区域 (x: 500 ~ 2000)
        "UAV_02": {
            "skill": "search",
            "params": {
                "search_area": [
                    [-1000, 1000],
                    [-2000, 1000],
                    [-2000, 2500],
                    [-500, 2500]
                ],
                "target": {
                    "class": "object",
                    "type": "box",
                    "features": {"color": "red", "label": "RedBox"}
                }
            }
        }
    },
    # "2": {
    #     # 搜索完成后返航
    #     "UAV_01": {"skill": "return_home", "params": {}},
    #     "UAV_02": {"skill": "return_home", "params": {}}
    # }
}

# ========== UGV+Humanoid 搬运技能列表 (第一阶段：导航到目标) ==========
# 需要从搜索结果中获取 RedBox 位置，或使用默认位置

def create_transport_skill_list_phase1(redbox_position=None):
    """创建搬运任务第一阶段：导航到 RedBox 位置"""
    if redbox_position is None:
        redbox_position = DEFAULT_REDBOX_POSITION
    
    # UGV 和 Humanoid 导航到 RedBox 附近
    # Humanoid 需要更靠近物体以便拾取
    return {
        "0": {
            "UGV_01": {
                "skill": "navigate",
                "params": {"dest": {"x": redbox_position["x"] + 100, "y": redbox_position["y"] - 400, "z": 0}}
            },
            "Humanoid_01": {
                "skill": "navigate",
                "params": {"dest": {"x": redbox_position["x"] - 100, "y": redbox_position["y"] - 200, "z": 0}}
            },
            "UAV_01": {"skill": "return_home", "params": {}},
            "UAV_02": {"skill": "return_home", "params": {}}   
        },
        "1": {
            # Humanoid 执行 place 技能：将 RedBox 放到 UGV 上
            "Humanoid_01": {
                "skill": "place",
                "params": {
                    "object1": {
                        "class": "object",
                        "type": "box",
                        "features": {"color": "red", "label": "RedBox"}
                    },
                    "object2": {
                        "class": "robot",
                        "type": "UGV",
                        "features": {"label": "UGV_01"}
                    }
                }
            }
        }
    }

# ========== UGV+Humanoid 搬运技能列表 (第二阶段：运输到终点并卸货) ==========

SKILL_LIST_TRANSPORT_PHASE2 = {
    "0": {
        # UGV 和 Humanoid 导航到终点
        "UGV_01": {
            "skill": "navigate",
            "params": {"dest": {"x": -5100, "y": 2200, "z": 0}}
        },
        "Humanoid_01": {
            "skill": "navigate",
            "params": {"dest": {"x": -5200, "y": 2100, "z": 0}}
        }
    },
    "1": {
        # Humanoid 执行 place 技能：将 RedBox 从 UGV 卸到地面
        "Humanoid_01": {
            "skill": "place",
            "params": {
                "object1": {
                    "class": "object",
                    "type": "box",
                    "features": {"color": "red", "label": "RedBox"}
                },
                "object2": {
                    "class": "ground",
                    "type": "",
                    "features": {}
                }
            }
        }
    }
}

# ========== Place 技能测试列表 ==========

# Place 测试 - 装货到 UGV
SKILL_LIST_PLACE_LOAD = {
    "0": {
        "Humanoid_01": {
            "skill": "place",
            "params": {
                "object1": {
                    "class": "object",
                    "type": "box",
                    "features": {"color": "red", "label": "RedBox"}
                },
                "object2": {
                    "class": "robot",
                    "type": "UGV",
                    "features": {"label": "UGV_01"}
                }
            }
        }
    }
}

# Place 测试 - 卸货到地面
SKILL_LIST_PLACE_UNLOAD = {
    "0": {
        "Humanoid_01": {
            "skill": "place",
            "params": {
                "object1": {
                    "class": "object",
                    "type": "box",
                    "features": {"color": "red", "label": "RedBox"}
                },
                "object2": {
                    "class": "ground",
                    "type": "",
                    "features": {}
                }
            }
        }
    }
}

# Place 测试 - UGV 和 Humanoid 协作搬运场景
SKILL_LIST_PLACE_COOP = {
    "0": {
        "UGV_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 400, "y": 1300, "z": 0}}
        },
        "Humanoid_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 200, "y": 1850, "z": 0}}
        }
    },
    "1": {
        "Humanoid_01": {
            "skill": "place",
            "params": {
                "object1": {
                    "class": "object",
                    "type": "box",
                    "features": {"color": "red", "label": "RedBox"}
                },
                "object2": {
                    "class": "robot",
                    "type": "UGV",
                    "features": {"label": "UGV_01"}
                }
            }
        }
    },
    "2": {
        "UGV_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 1000, "y": 1000, "z": 0}}
        }
    },
    "3": {
        "Humanoid_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 1150, "y": 1000, "z": 0}}
        }
    },
    "4": {
        "Humanoid_01": {
            "skill": "place",
            "params": {
                "object1": {
                    "class": "object",
                    "type": "box",
                    "features": {"color": "red", "label": "RedBox"}
                },
                "object2": {
                    "class": "ground",
                    "type": "",
                    "features": {}
                }
            }
        }
    }
}


class AutoTransportState:
    """自动搬运测试的状态管理"""
    def __init__(self):
        self.phase = 0  # 0: 等待搜索完成, 1: 等待搬运第一阶段完成, 2: 等待搬运第二阶段完成
        self.redbox_position = None
        self.search_completed = False
        self.found_target = False


class SimPollHandler(BaseHTTPRequestHandler):
    """处理 UE5 仿真端的轮询请求和反馈"""
    
    pending_messages = []
    received_feedbacks = []
    message_lock = threading.Lock()
    
    # 自动搬运测试状态
    auto_transport_state = None
    test_mode = None
    
    def do_GET(self):
        if self.path == '/api/sim/poll':
            with SimPollHandler.message_lock:
                if SimPollHandler.pending_messages:
                    msg = SimPollHandler.pending_messages.pop(0)
                    response = json.dumps([msg])
                    self.send_response(200)
                    self.send_header('Content-Type', 'application/json')
                    self.end_headers()
                    self.wfile.write(response.encode('utf-8'))
                    print(f"\n[{self._timestamp()}] >>> Sent skill list to UE5")
                else:
                    self.send_response(204)
                    self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        if self.path == '/api/sim/message':
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            
            try:
                message = json.loads(post_data.decode('utf-8'))
                self._handle_message(message)
                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(b'{"status": "ok"}')
            except json.JSONDecodeError:
                self.send_response(400)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()
    
    def _handle_message(self, message: dict):
        msg_type = message.get('message_type', '')
        payload = message.get('payload', {})
        
        if msg_type == 'task_feedback':
            self._handle_feedback(payload)
        else:
            print(f"\n[{self._timestamp()}] <<< Received: {msg_type}")
    
    def _handle_feedback(self, payload: dict):
        # 检查是否是技能列表完成反馈
        feedback_type = payload.get('feedback_type', '')
        
        if feedback_type == 'skill_list_completed':
            self._handle_skill_list_completed(payload)
            return
        
        # 普通时间步反馈
        time_step = payload.get('time_step', -1)
        feedbacks = payload.get('feedbacks', [])
        
        print(f"\n[{self._timestamp()}] <<< TimeStep {time_step} Feedback:")
        print("=" * 60)
        
        for fb in feedbacks:
            agent_id = fb.get('agent_id', 'Unknown')
            skill = fb.get('skill', 'Unknown')
            success = fb.get('success', False)
            message = fb.get('message', '')
            data = fb.get('data', {})
            
            status_icon = "✓" if success else "✗"
            status_text = "SUCCESS" if success else "FAILED"
            color = "\033[92m" if success else "\033[93m"
            
            print(f"  {color}{status_icon} [{agent_id}] {skill} - {status_text}\033[0m")
            print(f"    Message: {message}")
            
            # 如果是搜索技能，提取找到的对象信息
            if skill == 'Search' and success:
                found_count = data.get('found_count', '0')
                if int(found_count) > 0:
                    print(f"    Found {found_count} object(s):")
                    for i in range(int(found_count)):
                        obj_name = data.get(f'object_{i}_name', 'Unknown')
                        obj_x = data.get(f'object_{i}_x', '?')
                        obj_y = data.get(f'object_{i}_y', '?')
                        obj_z = data.get(f'object_{i}_z', '?')
                        print(f"      - {obj_name} at ({obj_x}, {obj_y}, {obj_z})")
                        
                        # 保存找到的 RedBox 位置用于自动搬运测试
                        if 'RedBox' in obj_name or 'redbox' in obj_name.lower():
                            try:
                                if SimPollHandler.auto_transport_state:
                                    SimPollHandler.auto_transport_state.redbox_position = {
                                        "x": float(obj_x),
                                        "y": float(obj_y),
                                        "z": float(obj_z)
                                    }
                                    SimPollHandler.auto_transport_state.found_target = True
                                    print(f"    \033[96m>>> Saved RedBox position for transport task\033[0m")
                            except ValueError:
                                pass
        
        print("=" * 60)
        SimPollHandler.received_feedbacks.append(payload)

    def _handle_skill_list_completed(self, payload: dict):
        """处理技能列表执行完成反馈"""
        completed = payload.get('completed', False)
        interrupted = payload.get('interrupted', False)
        completed_steps = payload.get('completed_time_steps', 0)
        total_steps = payload.get('total_time_steps', 0)
        message = payload.get('message', '')
        
        print(f"\n[{self._timestamp()}] <<< Skill List Execution Finished:")
        print("=" * 60)
        
        if completed:
            print(f"  \033[92m✓ COMPLETED\033[0m: {completed_steps}/{total_steps} time steps")
        elif interrupted:
            print(f"  \033[93m⚠ INTERRUPTED\033[0m: {completed_steps}/{total_steps} time steps")
        else:
            print(f"  \033[91m✗ ENDED\033[0m: {completed_steps}/{total_steps} time steps")
        
        print(f"  Message: {message}")
        print("=" * 60)
        
        # 自动搬运测试模式：根据反馈发送下一个技能列表
        if SimPollHandler.test_mode == 'transport_auto' and SimPollHandler.auto_transport_state:
            self._handle_auto_transport_next_phase(completed, interrupted)
    
    def _handle_auto_transport_next_phase(self, completed: bool, interrupted: bool):
        """自动搬运测试：根据当前阶段发送下一个技能列表"""
        state = SimPollHandler.auto_transport_state
        
        if state.phase == 0:
            # 搜索阶段完成，发送搬运第一阶段
            state.search_completed = True
            
            if state.found_target and state.redbox_position:
                print(f"\n\033[96m>>> Search completed, found RedBox at {state.redbox_position}\033[0m")
                print(f"\033[96m>>> Sending transport phase 1 (navigate to RedBox + load to UGV)\033[0m\n")
                redbox_pos = state.redbox_position
            else:
                print(f"\n\033[93m>>> Search completed but RedBox not found, using default position\033[0m")
                print(f"\033[96m>>> Sending transport phase 1 (navigate to RedBox + load to UGV)\033[0m\n")
                redbox_pos = DEFAULT_REDBOX_POSITION
            
            # 创建并发送搬运第一阶段技能列表
            skill_list = create_transport_skill_list_phase1(redbox_pos)
            print_skill_list("Transport Phase 1", skill_list)
            
            msg = create_skill_list_message(skill_list)
            with SimPollHandler.message_lock:
                SimPollHandler.pending_messages.append(msg)
            
            state.phase = 1
            
        elif state.phase == 1:
            # 搬运第一阶段完成，发送第二阶段
            print(f"\n\033[96m>>> Transport phase 1 completed\033[0m")
            print(f"\033[96m>>> Sending transport phase 2 (navigate to destination + unload)\033[0m\n")
            
            print_skill_list("Transport Phase 2", SKILL_LIST_TRANSPORT_PHASE2)
            
            msg = create_skill_list_message(SKILL_LIST_TRANSPORT_PHASE2)
            with SimPollHandler.message_lock:
                SimPollHandler.pending_messages.append(msg)
            
            state.phase = 2
            
        elif state.phase == 2:
            # 搬运第二阶段完成，任务结束
            print(f"\n\033[92m>>> Transport task completed!\033[0m")
            print(f"\033[92m>>> All phases finished successfully.\033[0m\n")
    
    def _timestamp(self):
        return datetime.now().strftime("%H:%M:%S")
    
    def log_message(self, format, *args):
        pass


def create_skill_list_message(skill_list: dict) -> dict:
    return {
        "message_type": "skill_list",
        "timestamp": int(datetime.now().timestamp() * 1000),
        "message_id": str(uuid.uuid4()),
        "payload": skill_list
    }


def print_skill_list(label: str, skill_list: dict):
    """打印技能列表内容"""
    print(f"\n{label}:")
    for step, cmds in sorted(skill_list.items(), key=lambda x: int(x[0])):
        print(f"  Step {step}:")
        for agent, cmd in cmds.items():
            skill = cmd.get('skill')
            params = cmd.get('params', {})
            dest = params.get('dest')
            object1 = params.get('object1')
            object2 = params.get('object2')
            search_area = params.get('search_area')
            target = params.get('target')
            
            if dest:
                print(f"    {agent}: {skill} -> ({dest['x']:.0f}, {dest['y']:.0f}, {dest['z']:.0f})")
            elif search_area and target:
                # target 的 label 在 features 里面
                target_features = target.get('features', {})
                target_label = target_features.get('label', target.get('label', target.get('id', 'unknown')))
                print(f"    {agent}: {skill} (area: {len(search_area)} vertices, target: {target_label})")
            elif object1 and object2:
                obj1_label = object1.get('features', {}).get('label', object1.get('type', 'unknown'))
                obj2_class = object2.get('class', '')
                if obj2_class == 'ground':
                    obj2_label = 'ground'
                elif obj2_class == 'robot':
                    obj2_label = object2.get('features', {}).get('label', 'UGV')
                else:
                    obj2_label = object2.get('features', {}).get('label', object2.get('type', 'unknown'))
                print(f"    {agent}: {skill} ({obj1_label} -> {obj2_label})")
            else:
                print(f"    {agent}: {skill}")


def run_server(port: int, test_mode: str, interval: float):
    """运行服务器"""
    
    print(f"{'='*60}")
    print(f"Skill List Server - Test Mode: {test_mode}")
    print(f"Port: {port}")
    print(f"{'='*60}")
    
    # 设置测试模式
    SimPollHandler.test_mode = test_mode
    
    # 根据测试模式设置技能列表
    if test_mode == 'single':
        print_skill_list("Skill List (Single)", SKILL_LIST_SINGLE)
        msg = create_skill_list_message(SKILL_LIST_SINGLE)
        SimPollHandler.pending_messages.append(msg)
        
    elif test_mode == 'uav_search':
        print_skill_list("UAV Cooperative Search", SKILL_LIST_UAV_SEARCH)
        msg = create_skill_list_message(SKILL_LIST_UAV_SEARCH)
        SimPollHandler.pending_messages.append(msg)
        
    elif test_mode == 'transport_auto':
        # 自动搬运测试：先发送搜索任务，然后根据反馈自动发送搬运任务
        SimPollHandler.auto_transport_state = AutoTransportState()
        
        print("\n\033[96m=== Auto Transport Test Mode ===\033[0m")
        print("Phase 0: UAV search for RedBox")
        print("Phase 1: Navigate to RedBox + Load to UGV (triggered by search completion)")
        print("Phase 2: Navigate to destination + Unload (triggered by phase 1 completion)")
        print("")
        
        print_skill_list("Phase 0: UAV Search", SKILL_LIST_UAV_SEARCH)
        msg = create_skill_list_message(SKILL_LIST_UAV_SEARCH)
        SimPollHandler.pending_messages.append(msg)
        
    elif test_mode == 'place_load':
        print_skill_list("Place Test - Load to UGV", SKILL_LIST_PLACE_LOAD)
        msg = create_skill_list_message(SKILL_LIST_PLACE_LOAD)
        SimPollHandler.pending_messages.append(msg)
        
    elif test_mode == 'place_unload':
        print_skill_list("Place Test - Unload to Ground", SKILL_LIST_PLACE_UNLOAD)
        msg = create_skill_list_message(SKILL_LIST_PLACE_UNLOAD)
        SimPollHandler.pending_messages.append(msg)
        
    elif test_mode == 'place_coop':
        print_skill_list("Place Test - UGV & Humanoid Cooperation", SKILL_LIST_PLACE_COOP)
        msg = create_skill_list_message(SKILL_LIST_PLACE_COOP)
        SimPollHandler.pending_messages.append(msg)
    
    print(f"\nWaiting for UE5 to poll...")
    print(f"Press Ctrl+C to stop\n")
    
    server = HTTPServer(('0.0.0.0', port), SimPollHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print(f"\n\nServer stopped. Received {len(SimPollHandler.received_feedbacks)} feedbacks.")


def main():
    parser = argparse.ArgumentParser(description='Send skill list to UE5')
    parser.add_argument('--server', action='store_true', help='Run as HTTP server')
    parser.add_argument('--port', type=int, default=8081, help='Server port')
    parser.add_argument('--test', type=str, default='single', 
                        choices=['single', 'uav_search', 'transport_auto',
                                 'place_load', 'place_unload', 'place_coop'],
                        help='Test mode')
    parser.add_argument('--interval', type=float, default=15.0,
                        help='Interval between skill lists in seconds (default: 15)')
    parser.add_argument('--print', action='store_true', help='Print skill list JSON')
    
    args = parser.parse_args()
    
    if args.print:
        print("=== UAV Search ===")
        msg = create_skill_list_message(SKILL_LIST_UAV_SEARCH)
        print(json.dumps(msg, indent=2, ensure_ascii=False))
        print("\n=== Transport Phase 1 ===")
        msg = create_skill_list_message(create_transport_skill_list_phase1())
        print(json.dumps(msg, indent=2, ensure_ascii=False))
        print("\n=== Transport Phase 2 ===")
        msg = create_skill_list_message(SKILL_LIST_TRANSPORT_PHASE2)
        print(json.dumps(msg, indent=2, ensure_ascii=False))
    elif args.server:
        run_server(args.port, args.test, args.interval)
    else:
        parser.print_help()


if __name__ == '__main__':
    main()


# 使用示例:
# 
# # 测试 UAV 协同搜索
# python scripts/send_skill_list.py --server --test uav_search
#
# # 测试自动搬运（依据反馈自动发送下一阶段）
# python scripts/send_skill_list.py --server --test transport_auto
#
# # 测试 Place 技能 - UGV 和 Humanoid 协作搬运
# python scripts/send_skill_list.py --server --test place_coop
#
# # 打印技能列表 JSON
# python scripts/send_skill_list.py --print
