#!/usr/bin/env python3
"""
UE5 世界状态查询客户端

提供查询 UE5 仿真世界状态的 API：
- get_world_state() - 获取完整世界状态
- get_all_entities() - 获取所有实体
- get_all_robots() - 获取所有机器人
- get_all_props() - 获取所有道具
- get_boundaries() - 获取边界特征
- get_entity_by_id(id) - 根据 ID 获取实体

使用方法:
    python scripts/world_query.py --server --query-all
"""

import json
import argparse
import threading
import time
import os
from http.server import HTTPServer, BaseHTTPRequestHandler
from datetime import datetime
import uuid
from typing import Dict, Any, List, Optional


class WorldQueryClient:
    """世界状态查询客户端"""
    
    def __init__(self, port: int = 8080):
        self.port = port
        self.pending_queries: List[dict] = []
        self.responses: Dict[str, Any] = {}
        self.response_events: Dict[str, threading.Event] = {}
        self.server: Optional[HTTPServer] = None
        self.server_thread: Optional[threading.Thread] = None
        self._running = False
    
    def start_server(self):
        """启动 HTTP 服务器"""
        handler = self._create_handler()
        self.server = HTTPServer(('0.0.0.0', self.port), handler)
        self._running = True
        self.server_thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.server_thread.start()
        print(f"[WorldQuery] Server started on port {self.port}")
    
    def stop_server(self):
        """停止服务器"""
        self._running = False
        if self.server:
            self.server.shutdown()
    
    def _create_handler(self):
        """创建请求处理器"""
        client = self
        
        class Handler(BaseHTTPRequestHandler):
            def do_GET(self):
                if self.path == '/api/sim/poll':
                    if client.pending_queries:
                        response = json.dumps(client.pending_queries)
                        client.pending_queries = []
                        self.send_response(200)
                        self.send_header('Content-Type', 'application/json')
                        self.end_headers()
                        self.wfile.write(response.encode('utf-8'))
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
                        client._handle_response(message)
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
            
            def log_message(self, format, *args):
                pass
        
        return Handler
    
    def _handle_response(self, message: dict):
        """处理 UE5 返回的响应"""
        msg_type = message.get('message_type', '')
        payload = message.get('payload', {})
        
        if msg_type == 'world_state':
            query_type = payload.get('query_type', 'unknown')
            data = payload.get('data', {})
            
            print(f"[WorldQuery] Received response: {query_type}")
            self.responses[query_type] = data
            
            if query_type in self.response_events:
                self.response_events[query_type].set()
    
    def _send_query(self, query_type: str, params: Optional[Dict] = None) -> Optional[Any]:
        """发送查询并等待响应"""
        # 创建事件
        event = threading.Event()
        self.response_events[query_type] = event
        
        # 构建查询消息
        payload = {"query_type": query_type}
        if params:
            payload.update(params)
        
        message = {
            "message_type": "query_request",
            "timestamp": int(datetime.now().timestamp() * 1000),
            "message_id": str(uuid.uuid4()),
            "payload": payload
        }
        
        self.pending_queries.append(message)
        print(f"[WorldQuery] Sent query: {query_type}")
        
        # 等待响应 (最多 10 秒)
        if event.wait(timeout=10.0):
            return self.responses.get(query_type)
        else:
            print(f"[WorldQuery] Timeout waiting for: {query_type}")
            return None
    
    # ========== 公开 API ==========
    
    def get_world_state(self) -> Optional[Dict]:
        """获取完整世界状态"""
        return self._send_query("world_state")
    
    def get_all_entities(self) -> Optional[Dict]:
        """获取所有实体"""
        return self._send_query("all_entities")
    
    def get_all_robots(self) -> Optional[Dict]:
        """获取所有机器人"""
        return self._send_query("all_robots")
    
    def get_all_props(self) -> Optional[Dict]:
        """获取所有道具"""
        return self._send_query("all_props")
    
    def get_boundaries(self, categories: Optional[List[str]] = None) -> Optional[Dict]:
        """获取边界特征"""
        params = {}
        if categories:
            params["categories"] = categories
        return self._send_query("boundaries", params if params else None)
    
    def get_entity_by_id(self, entity_id: str) -> Optional[Dict]:
        """根据 ID 获取实体"""
        return self._send_query("entity_by_id", {"entity_id": entity_id})


def query_all_and_save(client: WorldQueryClient, output_dir: str = "datasets"):
    """查询所有 API 并保存结果"""
    os.makedirs(output_dir, exist_ok=True)
    
    results = {}
    
    # 等待 UE5 轮询
    print("\n[WorldQuery] Waiting for UE5 to poll...")
    time.sleep(2)
    
    # 查询所有 API
    apis = [
        ("world_state", lambda: client.get_world_state()),
        ("all_entities", lambda: client.get_all_entities()),
        ("all_robots", lambda: client.get_all_robots()),
        ("all_props", lambda: client.get_all_props()),
        ("boundaries", lambda: client.get_boundaries()),
    ]
    
    for name, func in apis:
        print(f"\n[WorldQuery] Querying: {name}")
        time.sleep(1)  # 等待 UE5 轮询
        result = func()
        if result:
            results[name] = result
            # 单独保存每个查询结果
            filepath = os.path.join(output_dir, f"query_{name}.json")
            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump(result, f, indent=2, ensure_ascii=False)
            print(f"  Saved to: {filepath}")
        else:
            print(f"  No data received")
    
    # 保存汇总结果
    summary_path = os.path.join(output_dir, "query_all_results.json")
    with open(summary_path, 'w', encoding='utf-8') as f:
        json.dump(results, f, indent=2, ensure_ascii=False)
    print(f"\n[WorldQuery] All results saved to: {summary_path}")
    
    return results


def run_interactive(client: WorldQueryClient):
    """交互式查询"""
    print("\nAvailable commands:")
    print("  world_state  - Get complete world state")
    print("  all_entities - Get all entities")
    print("  all_robots   - Get all robots")
    print("  all_props    - Get all props")
    print("  boundaries   - Get boundary features")
    print("  entity <id>  - Get entity by ID")
    print("  save         - Query all and save to files")
    print("  quit         - Exit")
    print()
    
    while True:
        try:
            cmd = input("> ").strip().lower()
            if not cmd:
                continue
            
            if cmd in ['quit', 'exit', 'q']:
                break
            elif cmd == 'world_state':
                result = client.get_world_state()
            elif cmd == 'all_entities':
                result = client.get_all_entities()
            elif cmd == 'all_robots':
                result = client.get_all_robots()
            elif cmd == 'all_props':
                result = client.get_all_props()
            elif cmd == 'boundaries':
                result = client.get_boundaries()
            elif cmd.startswith('entity '):
                entity_id = cmd[7:].strip()
                result = client.get_entity_by_id(entity_id)
            elif cmd == 'save':
                query_all_and_save(client)
                continue
            else:
                print(f"Unknown command: {cmd}")
                continue
            
            if result:
                print(json.dumps(result, indent=2, ensure_ascii=False))
            else:
                print("No data received")
                
        except KeyboardInterrupt:
            break
        except EOFError:
            break


def main():
    parser = argparse.ArgumentParser(description='UE5 World Query Client')
    parser.add_argument('--server', action='store_true', help='Run as HTTP server')
    parser.add_argument('--port', type=int, default=8080, help='Server port (default: 8080)')
    parser.add_argument('--query-all', action='store_true', help='Query all APIs and save to files')
    parser.add_argument('--output', type=str, default='datasets', help='Output directory for saved files')
    
    args = parser.parse_args()
    
    if args.server:
        client = WorldQueryClient(port=args.port)
        client.start_server()
        
        print("=" * 50)
        print(f"World Query Server started on port {args.port}")
        print("Waiting for UE5 to connect...")
        print("=" * 50)
        
        try:
            if args.query_all:
                # 自动查询所有 API 并保存
                query_all_and_save(client, args.output)
                print("\nPress Ctrl+C to exit or continue with interactive mode...")
            
            # 进入交互模式
            run_interactive(client)
            
        except KeyboardInterrupt:
            pass
        finally:
            client.stop_server()
            print("\nServer stopped.")
    else:
        parser.print_help()


if __name__ == '__main__':
    main()
