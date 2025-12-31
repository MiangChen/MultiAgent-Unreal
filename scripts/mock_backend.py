#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Mock Backend Server
For testing the communication module of MultiAgent-Unreal simulation platform

API Endpoints:
- POST /api/sim/message      - Receive simulation messages (UI input, button events, task feedback)
- POST /api/sim/scene_change - Receive scene change messages
- GET  /api/sim/poll         - Return pending messages (currently returns empty)

Usage:
    python3 scripts/mock_backend.py [--port 8080]
"""

import json
import argparse
from datetime import datetime
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse
import sys

# ANSI color codes
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
    """Return colored text"""
    return f"{color}{text}{Colors.ENDC}"

def print_separator():
    """Print separator line"""
    print(colored("=" * 80, Colors.CYAN))

def print_json(data, indent=2):
    """Format and print JSON"""
    try:
        if isinstance(data, str):
            data = json.loads(data)
        print(json.dumps(data, indent=indent, ensure_ascii=False))
    except:
        print(data)

class MockBackendHandler(BaseHTTPRequestHandler):
    """Mock backend request handler"""
    
    def log_message(self, format, *args):
        """Override default logging with custom format"""
        pass  # Disable default logging, use custom output
    
    def send_json_response(self, status_code, data):
        """Send JSON response"""
        self.send_response(status_code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        
        response = json.dumps(data, ensure_ascii=False)
        self.wfile.write(response.encode('utf-8'))
    
    def do_OPTIONS(self):
        """Handle CORS preflight request"""
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
    
    def do_GET(self):
        """Handle GET request"""
        parsed_path = urlparse(self.path)
        
        if parsed_path.path == '/api/sim/poll':
            self.handle_poll()
        else:
            self.send_json_response(404, {"error": "Not Found"})
    
    def do_POST(self):
        """Handle POST request"""
        parsed_path = urlparse(self.path)
        
        # Read request body
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8') if content_length > 0 else ''
        
        if parsed_path.path == '/api/sim/message':
            self.handle_message(body)
        elif parsed_path.path == '/api/sim/scene_change':
            self.handle_scene_change(body)
        else:
            self.send_json_response(404, {"error": "Not Found"})
    
    def handle_poll(self):
        """Handle poll request"""
        timestamp = datetime.now().strftime('%H:%M:%S')
        print(f"\n{colored('[POLL]', Colors.BLUE)} {timestamp} - Poll request")
        
        # Return empty array indicating no new messages
        self.send_json_response(200, [])
    
    def handle_message(self, body):
        """Handle message request"""
        timestamp = datetime.now().strftime('%H:%M:%S')
        
        print_separator()
        print(f"{colored('[MESSAGE]', Colors.GREEN)} {timestamp} - Received simulation message")
        print_separator()
        
        try:
            data = json.loads(body)
            
            # Extract message type
            msg_type = data.get('message_type', 'unknown')
            msg_id = data.get('message_id', 'N/A')
            msg_timestamp = data.get('timestamp', 'N/A')
            payload = data.get('payload', {})
            
            print(f"{colored('Message Type:', Colors.YELLOW)} {msg_type}")
            print(f"{colored('Message ID:', Colors.YELLOW)} {msg_id}")
            print(f"{colored('Timestamp:', Colors.YELLOW)} {msg_timestamp}")
            print(f"{colored('Payload:', Colors.YELLOW)}")
            print_json(payload)
            
            # Print additional info based on message type
            if msg_type == 'ui_input':
                source_id = payload.get('input_source_id', 'N/A')
                content = payload.get('input_content', 'N/A')
                print(f"\n{colored('>>> UI Input:', Colors.CYAN)}")
                print(f"    Source: {source_id}")
                print(f"    Content: {content}")
                
            elif msg_type == 'button_event':
                widget = payload.get('widget_name', 'N/A')
                btn_id = payload.get('button_id', 'N/A')
                btn_text = payload.get('button_text', 'N/A')
                print(f"\n{colored('>>> Button Event:', Colors.CYAN)}")
                print(f"    Widget: {widget}")
                print(f"    Button ID: {btn_id}")
                print(f"    Button Text: {btn_text}")
                
            elif msg_type == 'task_feedback':
                task_id = payload.get('task_id', 'N/A')
                status = payload.get('status', 'N/A')
                duration = payload.get('duration_seconds', 'N/A')
                energy = payload.get('energy_consumed', 'N/A')
                print(f"\n{colored('>>> Task Feedback:', Colors.CYAN)}")
                print(f"    Task ID: {task_id}")
                print(f"    Status: {status}")
                print(f"    Duration: {duration}s")
                print(f"    Energy: {energy}")
            
            elif msg_type == 'task_graph_submit':
                print(f"\n{colored('>>> Task Graph Submit:', Colors.CYAN)}")
                # payload structure: { "meta": {...}, "task_graph": { "nodes": [...], "edges": [...] } }
                if isinstance(payload, str):
                    try:
                        payload = json.loads(payload)
                    except:
                        print(f"    Raw data: {payload}")
                        payload = {}
                
                # Extract nodes and edges from task_graph field
                task_graph = payload.get('task_graph', payload)  # Compatible with old format
                nodes = task_graph.get('nodes', [])
                edges = task_graph.get('edges', [])
                
                print(f"    Node count: {len(nodes)}")
                print(f"    Edge count: {len(edges)}")
                
                if nodes:
                    print(f"    Node list:")
                    for node in nodes:
                        # Support task_id or id field
                        node_id = node.get('task_id', node.get('id', 'N/A'))
                        node_desc = node.get('description', 'N/A')
                        node_loc = node.get('location', '')
                        loc_str = f" @ {node_loc}" if node_loc else ""
                        print(f"      - {node_id}: {node_desc}{loc_str}")
                
                if edges:
                    print(f"    Edge list:")
                    for edge in edges:
                        from_id = edge.get('from', 'N/A')
                        to_id = edge.get('to', 'N/A')
                        edge_type = edge.get('type', 'sequential')
                        print(f"      - {from_id} -> {to_id} ({edge_type})")
            
        except json.JSONDecodeError as e:
            print(f"{colored('JSON parse error:', Colors.RED)} {e}")
            print(f"Raw data: {body}")
        
        print_separator()
        print()
        
        # Return success response
        self.send_json_response(200, {
            "status": "received",
            "message": "Message received"
        })
    
    def handle_scene_change(self, body):
        """Handle scene change message"""
        timestamp = datetime.now().strftime('%H:%M:%S')
        
        print_separator()
        print(f"{colored('[SCENE CHANGE]', Colors.YELLOW)} {timestamp} - Received scene change message")
        print_separator()
        
        try:
            data = json.loads(body)
            
            change_type = data.get('change_type', 'unknown')
            msg_id = data.get('message_id', 'N/A')
            msg_timestamp = data.get('timestamp', 'N/A')
            payload = data.get('payload', '')
            
            print(f"{colored('Change type:', Colors.YELLOW)} {change_type}")
            print(f"{colored('Message ID:', Colors.YELLOW)} {msg_id}")
            print(f"{colored('Timestamp:', Colors.YELLOW)} {msg_timestamp}")
            print(f"{colored('Payload:', Colors.YELLOW)}")
            
            # Try to parse payload as JSON
            try:
                payload_data = json.loads(payload) if isinstance(payload, str) else payload
                print_json(payload_data)
            except:
                print(payload)
            
            # Print additional info based on change type
            change_type_desc = {
                'add_node': 'Add node',
                'delete_node': 'Delete node',
                'edit_node': 'Edit node',
                'add_goal': 'Add Goal',
                'delete_goal': 'Delete Goal',
                'add_zone': 'Add Zone',
                'delete_zone': 'Delete Zone'
            }
            
            desc = change_type_desc.get(change_type, 'Unknown operation')
            print(f"\n{colored(f'>>> Scene Operation: {desc}', Colors.CYAN)}")
            
        except json.JSONDecodeError as e:
            print(f"{colored('JSON parse error:', Colors.RED)} {e}")
            print(f"Raw data: {body}")
        
        print_separator()
        print()
        
        # Return success response
        self.send_json_response(200, {
            "status": "received",
            "message": "Scene change received"
        })

def main():
    parser = argparse.ArgumentParser(description='MultiAgent-Unreal Mock Backend Server')
    parser.add_argument('--port', type=int, default=8080, help='Server port (default: 8080)')
    parser.add_argument('--host', type=str, default='0.0.0.0', help='Server address (default: 0.0.0.0)')
    args = parser.parse_args()
    
    server_address = (args.host, args.port)
    httpd = HTTPServer(server_address, MockBackendHandler)
    
    print_separator()
    print(colored("  MultiAgent-Unreal Mock Backend Server", Colors.BOLD + Colors.GREEN))
    print_separator()
    print(f"  Server address: {colored(f'http://{args.host}:{args.port}', Colors.CYAN)}")
    print(f"  API Endpoints:")
    print(f"    - POST /api/sim/message      (Receive messages)")
    print(f"    - POST /api/sim/scene_change (Scene changes)")
    print(f"    - GET  /api/sim/poll         (Poll)")
    print_separator()
    print(f"  {colored('Waiting for simulation platform connection...', Colors.YELLOW)}")
    print(f"  Press Ctrl+C to stop server")
    print_separator()
    print()
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print(f"\n{colored('Server stopped', Colors.RED)}")
        httpd.shutdown()
        sys.exit(0)

if __name__ == '__main__':
    main()
