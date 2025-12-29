#!/usr/bin/env python3
"""
发送技能列表到 UE5 仿真端并接收执行反馈

使用方法:
    # 测试单个技能列表
    python scripts/send_skill_list.py --server --test single
    
    # 测试连续两个技能列表（间隔15秒发送第二个）
    python scripts/send_skill_list.py --server --test double
    
    # 测试连续三个技能列表
    python scripts/send_skill_list.py --server --test triple
    
    # 自定义间隔时间（秒）
    python scripts/send_skill_list.py --server --test double --interval 10
"""

import json
import argparse
import threading
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime
import uuid

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

# 第一个技能列表（用于连续测试）：多个机器人同时导航
SKILL_LIST_FIRST = {
    "0": {
        "UAV_01": {"skill": "take_off", "params": {}},
        # "Humanoid_01": {
        #     "skill": "navigate",
        #     "params": {"dest": {"x": -2000, "y": 0, "z": 0}}
        # },
        # "UGV_01": {
        #     "skill": "navigate",
        #     "params": {"dest": {"x": 2000, "y": 2000, "z": 0}}
        # }
    },
    "1": {
        "UAV_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 3000, "y": 3000, "z": 3200}}
        },
        # "Humanoid_01": {
        #     "skill": "navigate",
        #     "params": {"dest": {"x": -2000, "y": 2000, "z": 0}}
        # }
    },
    "2": {
        "UAV_01": {
            "skill": "navigate",
            "params": {"dest": {"x": -3000, "y": -3000, "z": 800}}
        }
    }
}

# 第二个技能列表（中断第一个）：所有机器人返回原点附近
SKILL_LIST_SECOND = {
    "0": {
        "UAV_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 0, "y": 0, "z": 600}}
        },
        "Humanoid_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 400, "y": 600, "z": 0}}
        },
        "UGV_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 800, "y": 400, "z": 0}}
        }
    },
    "1": {
        "UAV_01": {"skill": "land", "params": {}}
    }
}

# 第三个技能列表（中断第二个）：新的任务
SKILL_LIST_THIRD = {
    "0": {
        "Humanoid_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 1000, "y": 1000, "z": 0}}
        },
        "Quadruped_01": {
            "skill": "navigate",
            "params": {"dest": {"x": -1000, "y": 1000, "z": 0}}
        }
    },
    "1": {
        "Humanoid_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 0, "y": 0, "z": 0}}
        },
        "Quadruped_01": {
            "skill": "navigate",
            "params": {"dest": {"x": 0, "y": 0, "z": 0}}
        }
    }
}


class SimPollHandler(BaseHTTPRequestHandler):
    """处理 UE5 仿真端的轮询请求和反馈"""
    
    pending_messages = []
    received_feedbacks = []
    message_lock = threading.Lock()
    
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
        time_step = payload.get('time_step', -1)
        feedbacks = payload.get('feedbacks', [])
        
        print(f"\n[{self._timestamp()}] <<< TimeStep {time_step} Feedback:")
        print("=" * 60)
        
        for fb in feedbacks:
            agent_id = fb.get('agent_id', 'Unknown')
            skill = fb.get('skill', 'Unknown')
            success = fb.get('success', False)
            message = fb.get('message', '')
            
            status_icon = "✓" if success else "✗"
            status_text = "SUCCESS" if success else "FAILED"
            color = "\033[92m" if success else "\033[93m"
            
            print(f"  {color}{status_icon} [{agent_id}] {skill} - {status_text}\033[0m")
            print(f"    Message: {message}")
        
        print("=" * 60)
        SimPollHandler.received_feedbacks.append(payload)
    
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


def print_skill_list(name: str, skill_list: dict):
    """打印技能列表内容"""
    print(f"\n{name}:")
    for step, cmds in sorted(skill_list.items(), key=lambda x: int(x[0])):
        print(f"  Step {step}:")
        for agent, cmd in cmds.items():
            skill = cmd.get('skill')
            params = cmd.get('params', {})
            dest = params.get('dest')
            if dest:
                print(f"    {agent}: {skill} -> ({dest['x']:.0f}, {dest['y']:.0f}, {dest['z']:.0f})")
            else:
                print(f"    {agent}: {skill}")


def schedule_skill_list(skill_list: dict, delay: float, name: str):
    """延迟发送技能列表"""
    def send():
        time.sleep(delay)
        print(f"\n{'='*60}")
        print(f"[{datetime.now().strftime('%H:%M:%S')}] Scheduling {name} (after {delay}s delay)")
        print(f"{'='*60}")
        print_skill_list(name, skill_list)
        
        msg = create_skill_list_message(skill_list)
        with SimPollHandler.message_lock:
            SimPollHandler.pending_messages.append(msg)
        print(f"\n[{datetime.now().strftime('%H:%M:%S')}] {name} queued, waiting for UE5 poll...")
    
    thread = threading.Thread(target=send, daemon=True)
    thread.start()


def run_server(port: int, test_mode: str, interval: float):
    """运行服务器"""
    
    print(f"{'='*60}")
    print(f"Skill List Server - Test Mode: {test_mode}")
    print(f"Port: {port}")
    if test_mode in ['double', 'triple']:
        print(f"Interval between skill lists: {interval}s")
    print(f"{'='*60}")
    
    # 根据测试模式设置技能列表
    if test_mode == 'single':
        print_skill_list("Skill List (Single)", SKILL_LIST_FIRST)
        msg = create_skill_list_message(SKILL_LIST_FIRST)
        SimPollHandler.pending_messages.append(msg)
        
    elif test_mode == 'double':
        print_skill_list("Skill List #1 (First)", SKILL_LIST_FIRST)
        msg = create_skill_list_message(SKILL_LIST_FIRST)
        SimPollHandler.pending_messages.append(msg)
        
        # 延迟发送第二个
        schedule_skill_list(SKILL_LIST_SECOND, interval, "Skill List #2 (Interrupt)")
        
    elif test_mode == 'triple':
        print_skill_list("Skill List #1 (First)", SKILL_LIST_FIRST)
        msg = create_skill_list_message(SKILL_LIST_FIRST)
        SimPollHandler.pending_messages.append(msg)
        
        # 延迟发送第二个和第三个
        schedule_skill_list(SKILL_LIST_SECOND, interval, "Skill List #2 (Interrupt)")
        schedule_skill_list(SKILL_LIST_THIRD, interval * 2, "Skill List #3 (Interrupt)")
    
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
    parser.add_argument('--port', type=int, default=8080, help='Server port')
    parser.add_argument('--test', type=str, default='single', 
                        choices=['single', 'double', 'triple'],
                        help='Test mode: single, double (2 lists), triple (3 lists)')
    parser.add_argument('--interval', type=float, default=15.0,
                        help='Interval between skill lists in seconds (default: 15)')
    parser.add_argument('--print', action='store_true', help='Print skill list JSON')
    
    args = parser.parse_args()
    
    if args.print:
        print("=== Single Test ===")
        msg = create_skill_list_message(SKILL_LIST_SINGLE)
        print(json.dumps(msg, indent=2, ensure_ascii=False))
    elif args.server:
        run_server(args.port, args.test, args.interval)
    else:
        parser.print_help()


if __name__ == '__main__':
    main()


# # 测试单个技能列表
# python scripts/send_skill_list.py --server --test single

# # 测试连续两个技能列表（默认间隔15秒）
# python scripts/send_skill_list.py --server --test double

# # 测试连续三个技能列表（间隔15秒和30秒）
# python scripts/send_skill_list.py --server --test triple

# # 自定义间隔时间（10秒）
# python scripts/send_skill_list.py --server --test double --interval 10

