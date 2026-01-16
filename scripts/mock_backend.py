#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Mock Backend Server with Web UI
For testing the communication module of MultiAgent-Unreal simulation platform

Features:
- Web UI at http://localhost:8080
- Left panel: Skill list buttons to send commands to UE5
- Right panel: Real-time display of messages from UE5
- WebSocket for real-time updates

API Endpoints:
- GET  /                     - Web UI
- GET  /api/sim/poll         - UE5 polls for pending messages
- POST /api/sim/message      - Receive messages from UE5
- POST /api/sim/scene_change - Receive scene change messages
- POST /api/send_skill       - Send skill list (from Web UI)
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
    "uav_search": {
        "name": "UAV Search",
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
    },
    "navigate_test": {
        "name": "Navigate Test",
        "description": "单个 Humanoid 导航测试",
        "data": {
            "0": {"Humanoid_01": {"skill": "navigate", "params": {"dest": {"x": 1000, "y": 1000, "z": 0}}}}
        }
    },
    "all_takeoff": {
        "name": "All UAV TakeOff",
        "description": "所有 UAV 起飞",
        "data": {
            "0": {
                "UAV_01": {"skill": "take_off", "params": {}},
                "UAV_02": {"skill": "take_off", "params": {}}
            }
        }
    },
    "all_land": {
        "name": "All UAV Land",
        "description": "所有 UAV 降落",
        "data": {
            "0": {
                "UAV_01": {"skill": "land", "params": {}},
                "UAV_02": {"skill": "land", "params": {}}
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
            width: 320px; background: #16213e; padding: 20px;
            border-right: 1px solid #0f3460; overflow-y: auto;
        }
        .left-panel h2 { color: #e94560; margin-bottom: 20px; font-size: 18px; }
        .skill-btn {
            width: 100%; padding: 12px 16px; margin-bottom: 10px;
            background: #0f3460; border: 1px solid #e94560; border-radius: 8px;
            color: #fff; cursor: pointer; text-align: left; transition: all 0.2s;
        }
        .skill-btn:hover { background: #e94560; transform: translateX(5px); }
        .skill-btn .name { font-weight: bold; font-size: 14px; }
        .skill-btn .desc { font-size: 12px; color: #aaa; margin-top: 4px; }
        .skill-btn:hover .desc { color: #fff; }
        
        .section-title { 
            color: #00d9ff; font-size: 14px; margin: 20px 0 10px; 
            padding-bottom: 5px; border-bottom: 1px solid #0f3460;
        }
        
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
            <h2>🎮 Skill Commands</h2>
            
            <div class="section-title">📋 预设技能列表</div>
            <div id="skill-buttons"></div>
            
            <div class="section-title">⚡ 快捷命令</div>
            <button class="skill-btn" onclick="sendSkill('all_takeoff')">
                <div class="name">🚁 All UAV TakeOff</div>
                <div class="desc">所有无人机起飞</div>
            </button>
            <button class="skill-btn" onclick="sendSkill('all_land')">
                <div class="name">🛬 All UAV Land</div>
                <div class="desc">所有无人机降落</div>
            </button>
            
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
        const messagesDiv = document.getElementById('messages');
        const emptyState = document.getElementById('empty-state');
        let messageCount = 0;
        
        // Generate skill buttons
        const skillButtonsDiv = document.getElementById('skill-buttons');
        for (const [key, skill] of Object.entries(skillLists)) {
            if (key === 'all_takeoff' || key === 'all_land') continue;
            const btn = document.createElement('button');
            btn.className = 'skill-btn';
            btn.onclick = () => sendSkill(key);
            btn.innerHTML = `<div class="name">${skill.name}</div><div class="desc">${skill.description}</div>`;
            skillButtonsDiv.appendChild(btn);
        }
        
        // Send skill list
        async function sendSkill(skillKey) {
            try {
                const response = await fetch('/api/send_skill', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ skill_key: skillKey })
                });
                const result = await response.json();
                addMessage('sent', `Sent: ${skillLists[skillKey]?.name || skillKey}`, result);
            } catch (e) {
                console.error('Failed to send skill:', e);
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
        else:
            self.send_response(404)
            self.end_headers()
    
    def serve_html(self):
        """Serve the Web UI"""
        global server_port
        html = HTML_TEMPLATE.replace('{{PORT}}', str(server_port))
        html = html.replace('{{SKILL_LISTS}}', json.dumps(SKILL_LISTS, ensure_ascii=False))
        
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
                broadcast_message('sent', 'skill_list', msg, 'outgoing')
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
        """Handle skill send request from Web UI"""
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
                    "message_id": msg['message_id']
                })
            else:
                self.send_json_response(400, {"error": f"Unknown skill: {skill_key}"})
                
        except json.JSONDecodeError as e:
            self.send_json_response(400, {"error": str(e)})


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
