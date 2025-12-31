#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
模拟后端服务器
用于测试 MultiAgent-Unreal 仿真平台的通信模块

API 端点:
- POST /api/sim/message      - 接收仿真端消息 (UI 输入、按钮事件、任务反馈)
- POST /api/sim/scene_change - 接收场景变化消息
- GET  /api/sim/poll         - 返回待处理消息 (目前返回空)

启动方式:
    python3 scripts/mock_backend.py [--port 8080]
"""

import json
import argparse
from datetime import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse
import sys

# ANSI 颜色代码
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'

def colored(text, color):
    """返回带颜色的文本"""
    return f"{color}{text}{Colors.ENDC}"

def print_separator():
    """打印分隔线"""
    print(colored("=" * 80, Colors.CYAN))

def print_json(data, indent=2):
    """格式化打印 JSON"""
    try:
        if isinstance(data, str):
            data = json.loads(data)
        print(json.dumps(data, indent=indent, ensure_ascii=False))
    except:
        print(data)

class MockBackendHandler(BaseHTTPRequestHandler):
    """模拟后端请求处理器"""
    
    def log_message(self, format, *args):
        """覆盖默认日志，使用自定义格式"""
        pass  # 禁用默认日志，使用自定义输出
    
    def send_json_response(self, status_code, data):
        """发送 JSON 响应"""
        self.send_response(status_code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        
        response = json.dumps(data, ensure_ascii=False)
        self.wfile.write(response.encode('utf-8'))
    
    def do_OPTIONS(self):
        """处理 CORS 预检请求"""
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
    
    def do_GET(self):
        """处理 GET 请求"""
        parsed_path = urlparse(self.path)
        
        if parsed_path.path == '/api/sim/poll':
            self.handle_poll()
        else:
            self.send_json_response(404, {"error": "Not Found"})
    
    def do_POST(self):
        """处理 POST 请求"""
        parsed_path = urlparse(self.path)
        
        # 读取请求体
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8') if content_length > 0 else ''
        
        if parsed_path.path == '/api/sim/message':
            self.handle_message(body)
        elif parsed_path.path == '/api/sim/scene_change':
            self.handle_scene_change(body)
        else:
            self.send_json_response(404, {"error": "Not Found"})
    
    def handle_poll(self):
        """处理轮询请求"""
        timestamp = datetime.now().strftime('%H:%M:%S')
        print(f"\n{colored('[POLL]', Colors.BLUE)} {timestamp} - 轮询请求")
        
        # 返回空数组表示没有新消息
        self.send_json_response(200, [])
    
    def handle_message(self, body):
        """处理消息请求"""
        timestamp = datetime.now().strftime('%H:%M:%S')
        
        print_separator()
        print(f"{colored('[MESSAGE]', Colors.GREEN)} {timestamp} - 收到仿真端消息")
        print_separator()
        
        try:
            data = json.loads(body)
            
            # 提取消息类型
            msg_type = data.get('message_type', 'unknown')
            msg_id = data.get('message_id', 'N/A')
            msg_timestamp = data.get('timestamp', 'N/A')
            payload = data.get('payload', {})
            
            print(f"{colored('消息类型:', Colors.YELLOW)} {msg_type}")
            print(f"{colored('消息 ID:', Colors.YELLOW)} {msg_id}")
            print(f"{colored('时间戳:', Colors.YELLOW)} {msg_timestamp}")
            print(f"{colored('Payload:', Colors.YELLOW)}")
            print_json(payload)
            
            # 根据消息类型打印额外信息
            if msg_type == 'ui_input':
                source_id = payload.get('input_source_id', 'N/A')
                content = payload.get('input_content', 'N/A')
                print(f"\n{colored('>>> UI 输入:', Colors.CYAN)}")
                print(f"    来源: {source_id}")
                print(f"    内容: {content}")
                
            elif msg_type == 'button_event':
                widget = payload.get('widget_name', 'N/A')
                btn_id = payload.get('button_id', 'N/A')
                btn_text = payload.get('button_text', 'N/A')
                print(f"\n{colored('>>> 按钮事件:', Colors.CYAN)}")
                print(f"    Widget: {widget}")
                print(f"    按钮 ID: {btn_id}")
                print(f"    按钮文字: {btn_text}")
                
            elif msg_type == 'task_feedback':
                task_id = payload.get('task_id', 'N/A')
                status = payload.get('status', 'N/A')
                duration = payload.get('duration_seconds', 'N/A')
                energy = payload.get('energy_consumed', 'N/A')
                print(f"\n{colored('>>> 任务反馈:', Colors.CYAN)}")
                print(f"    任务 ID: {task_id}")
                print(f"    状态: {status}")
                print(f"    耗时: {duration}s")
                print(f"    能耗: {energy}")
            
            elif msg_type == 'task_graph_submit':
                print(f"\n{colored('>>> 任务图提交:', Colors.CYAN)}")
                # payload 结构: { "meta": {...}, "task_graph": { "nodes": [...], "edges": [...] } }
                if isinstance(payload, str):
                    try:
                        payload = json.loads(payload)
                    except:
                        print(f"    原始数据: {payload}")
                        payload = {}
                
                # 从 task_graph 字段中提取节点和边
                task_graph = payload.get('task_graph', payload)  # 兼容旧格式
                nodes = task_graph.get('nodes', [])
                edges = task_graph.get('edges', [])
                
                print(f"    节点数: {len(nodes)}")
                print(f"    边数: {len(edges)}")
                
                if nodes:
                    print(f"    节点列表:")
                    for node in nodes:
                        # 支持 task_id 或 id 字段
                        node_id = node.get('task_id', node.get('id', 'N/A'))
                        node_desc = node.get('description', 'N/A')
                        node_loc = node.get('location', '')
                        loc_str = f" @ {node_loc}" if node_loc else ""
                        print(f"      - {node_id}: {node_desc}{loc_str}")
                
                if edges:
                    print(f"    边列表:")
                    for edge in edges:
                        from_id = edge.get('from', 'N/A')
                        to_id = edge.get('to', 'N/A')
                        edge_type = edge.get('type', 'sequential')
                        print(f"      - {from_id} -> {to_id} ({edge_type})")
            
        except json.JSONDecodeError as e:
            print(f"{colored('JSON 解析错误:', Colors.RED)} {e}")
            print(f"原始数据: {body}")
        
        print_separator()
        print()
        
        # 返回成功响应
        self.send_json_response(200, {
            "status": "received",
            "message": "消息已接收"
        })
    
    def handle_scene_change(self, body):
        """处理场景变化消息"""
        timestamp = datetime.now().strftime('%H:%M:%S')
        
        print_separator()
        print(f"{colored('[SCENE CHANGE]', Colors.YELLOW)} {timestamp} - 收到场景变化消息")
        print_separator()
        
        try:
            data = json.loads(body)
            
            change_type = data.get('change_type', 'unknown')
            msg_id = data.get('message_id', 'N/A')
            msg_timestamp = data.get('timestamp', 'N/A')
            payload = data.get('payload', '')
            
            print(f"{colored('变化类型:', Colors.YELLOW)} {change_type}")
            print(f"{colored('消息 ID:', Colors.YELLOW)} {msg_id}")
            print(f"{colored('时间戳:', Colors.YELLOW)} {msg_timestamp}")
            print(f"{colored('Payload:', Colors.YELLOW)}")
            
            # 尝试解析 payload 为 JSON
            try:
                payload_data = json.loads(payload) if isinstance(payload, str) else payload
                print_json(payload_data)
            except:
                print(payload)
            
            # 根据变化类型打印额外信息
            change_type_desc = {
                'add_node': '添加节点',
                'delete_node': '删除节点',
                'edit_node': '编辑节点',
                'add_goal': '添加 Goal',
                'delete_goal': '删除 Goal',
                'add_zone': '添加 Zone',
                'delete_zone': '删除 Zone'
            }
            
            desc = change_type_desc.get(change_type, '未知操作')
            print(f"\n{colored(f'>>> 场景操作: {desc}', Colors.CYAN)}")
            
        except json.JSONDecodeError as e:
            print(f"{colored('JSON 解析错误:', Colors.RED)} {e}")
            print(f"原始数据: {body}")
        
        print_separator()
        print()
        
        # 返回成功响应
        self.send_json_response(200, {
            "status": "received",
            "message": "场景变化已接收"
        })

def main():
    parser = argparse.ArgumentParser(description='MultiAgent-Unreal 模拟后端服务器')
    parser.add_argument('--port', type=int, default=8080, help='服务器端口 (默认: 8080)')
    parser.add_argument('--host', type=str, default='0.0.0.0', help='服务器地址 (默认: 0.0.0.0)')
    args = parser.parse_args()
    
    server_address = (args.host, args.port)
    httpd = HTTPServer(server_address, MockBackendHandler)
    
    print_separator()
    print(colored("  MultiAgent-Unreal 模拟后端服务器", Colors.BOLD + Colors.GREEN))
    print_separator()
    print(f"  服务器地址: {colored(f'http://{args.host}:{args.port}', Colors.CYAN)}")
    print(f"  API 端点:")
    print(f"    - POST /api/sim/message      (接收消息)")
    print(f"    - POST /api/sim/scene_change (场景变化)")
    print(f"    - GET  /api/sim/poll         (轮询)")
    print_separator()
    print(f"  {colored('等待仿真平台连接...', Colors.YELLOW)}")
    print(f"  按 Ctrl+C 停止服务器")
    print_separator()
    print()
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print(f"\n{colored('服务器已停止', Colors.RED)}")
        httpd.shutdown()
        sys.exit(0)

if __name__ == '__main__':
    main()
