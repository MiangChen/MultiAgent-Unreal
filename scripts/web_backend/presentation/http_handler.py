from __future__ import annotations

import json
import queue
from http.server import BaseHTTPRequestHandler
from urllib.parse import urlparse

from web_backend.bootstrap.app_context import MockBackendAppContext
from web_backend.contexts.console.presentation.console_page import build_console_html
from web_backend.contexts.demo.presentation.demo_page import build_demo_page_html
from webui_asset_helpers import read_asset_bytes


class MockBackendHandler(BaseHTTPRequestHandler):
    """Presentation adapter for the mock backend HTTP API."""

    @property
    def app_context(self) -> MockBackendAppContext:
        return self.server.app_context  # type: ignore[attr-defined]

    @property
    def catalog(self):
        return self.app_context.catalog

    @property
    def runtime(self):
        return self.app_context.runtime

    @property
    def command_dispatch(self):
        return self.app_context.command_dispatch

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
        elif parsed_path.path == '/demo':
            self.serve_demo_html()
        elif parsed_path.path.startswith('/assets/'):
            self.serve_asset(parsed_path.path[len('/assets/'):])
        elif parsed_path.path == '/api/sim/poll':
            self.handle_poll()
        elif parsed_path.path == '/api/hitl/poll':
            self.handle_hitl_poll()
        elif parsed_path.path == '/api/health':
            self.handle_health()
        elif parsed_path.path == '/api/ue_status':
            self.handle_ue_status()
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
        elif parsed_path.path == '/api/hitl/message':
            self.handle_hitl_message(body)
        elif parsed_path.path == '/api/sim/scene_change':
            self.handle_scene_change(body)
        elif parsed_path.path == '/api/send_skill':
            self.handle_send_skill(body)
        elif parsed_path.path == '/api/send_skill_allocation':
            self.handle_send_skill_allocation(body)
        elif parsed_path.path == '/api/send_task_graph':
            self.handle_send_task_graph(body)
        elif parsed_path.path == '/api/send_request_user_command':
            self.handle_send_request_user_command()
        elif parsed_path.path == '/api/demo/send_step':
            self.handle_demo_send_step(body)
        else:
            self.send_response(404)
            self.end_headers()

    def serve_html(self):
        html = build_console_html(
            port=self.server.server_address[1],
            skill_lists=self.catalog.skill_lists,
            task_graphs=self.catalog.task_graphs,
            skill_allocations=self.catalog.skill_allocations,
        )
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(html.encode('utf-8'))

    def serve_demo_html(self):
        html = build_demo_page_html(
            skill_lists=self.catalog.skill_lists,
            skill_allocations=self.catalog.skill_allocations,
        )
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(html.encode('utf-8'))

    def serve_asset(self, asset_relative_path: str):
        asset = read_asset_bytes(asset_relative_path)
        if asset is None:
            self.send_response(404)
            self.end_headers()
            return

        body, content_type = asset
        self.send_response(200)
        self.send_header('Content-Type', content_type)
        self.end_headers()
        self.wfile.write(body)

    def handle_demo_send_step(self, body: str):
        try:
            data = json.loads(body)
            status, response = self.app_context.demo_action.handle_demo_action(
                action_type=data.get('action_type', ''),
                action_key=data.get('action_key', ''),
            )
            self.send_json_response(status, response)
        except json.JSONDecodeError as exc:
            self.send_json_response(400, {'error': str(exc)})

    def handle_poll(self):
        self.runtime.mark_ue_heartbeat()
        message = self.runtime.dequeue_platform_message()
        if message is None:
            self.send_response(204)
            self.end_headers()
            return

        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps([message], ensure_ascii=False).encode('utf-8'))
        message_type = message.get('message_type', 'unknown')
        self.runtime.broadcast_message('sent', f'Platform: {message_type}', message, 'outgoing')

    def handle_hitl_poll(self):
        self.runtime.mark_ue_heartbeat()
        print(f"[HITL Poll] UE5 polling HITL endpoint, queue size: {self.runtime.hitl_queue_size()}")
        message = self.runtime.dequeue_hitl_message()
        if message is None:
            self.send_response(204)
            self.end_headers()
            return

        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps([message], ensure_ascii=False).encode('utf-8'))
        message_type = message.get('message_type', 'unknown')
        message_category = message.get('message_category', 'unknown')
        print(f"[HITL Poll] ✅ Sent message to UE5: type={message_type}, category={message_category}")
        self.runtime.broadcast_message('sent', f'HITL ({message_category}): {message_type}', message, 'outgoing')

    def handle_health(self):
        self.send_json_response(200, {'status': 'healthy'})

    def handle_ue_status(self):
        connected, age = self.runtime.get_ue_connection_state()
        self.send_json_response(200, {
            'connected': connected,
            'last_poll_seconds_ago': round(age, 3) if age is not None else None,
            'timeout_seconds': self.runtime.ue_connection_timeout_seconds,
        })

    def handle_sse(self):
        self.send_response(200)
        self.send_header('Content-Type', 'text/event-stream')
        self.send_header('Cache-Control', 'no-cache')
        self.send_header('Connection', 'keep-alive')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()

        try:
            while True:
                try:
                    data = self.runtime.message_queue.get(timeout=30)
                    self.wfile.write(f'data: {data}\n\n'.encode('utf-8'))
                    self.wfile.flush()
                except queue.Empty:
                    self.wfile.write(': heartbeat\n\n'.encode('utf-8'))
                    self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            pass

    def handle_message(self, body: str):
        try:
            data = json.loads(body)
            message_type = data.get('message_type', 'unknown')
            self.runtime.broadcast_message('message', f'Platform: {message_type}', data, 'incoming')
            self.send_json_response(200, {'status': 'received'})
        except json.JSONDecodeError as exc:
            self.runtime.broadcast_message('error', 'JSON Parse Error', str(exc), 'incoming')
            self.send_json_response(400, {'error': str(exc)})

    def handle_hitl_message(self, body: str):
        try:
            data = json.loads(body)
            message_type = data.get('message_type', 'unknown')
            message_category = data.get('message_category', 'unknown')
            payload = data.get('payload', {})

            self.runtime.append_hitl_response(data)

            approved = payload.get('approved', None)
            status_str = ''
            if approved is not None:
                status_str = ' ✓ APPROVED' if approved else ' ✗ REJECTED'

            self.runtime.broadcast_message('hitl_response', f'HITL ({message_category}): {message_type}{status_str}', data, 'incoming')
            self.app_context.review_workflow.handle_review_response(data)
            self.send_json_response(200, {'status': 'received'})
        except json.JSONDecodeError as exc:
            self.runtime.broadcast_message('error', 'HITL JSON Parse Error', str(exc), 'incoming')
            self.send_json_response(400, {'error': str(exc)})

    def handle_scene_change(self, body: str):
        try:
            data = json.loads(body)
            change_type = data.get('change_type', 'unknown')
            self.runtime.broadcast_message('scene_change', f'scene_change: {change_type}', data, 'incoming')
            self.send_json_response(200, {'status': 'received'})
        except json.JSONDecodeError as exc:
            self.send_json_response(400, {'error': str(exc)})

    def handle_send_skill(self, body: str):
        try:
            data = json.loads(body)
            status, response = self.command_dispatch.queue_skill_list(data.get('skill_key', ''))
            self.send_json_response(status, response)
        except json.JSONDecodeError as exc:
            self.send_json_response(400, {'error': str(exc)})

    def handle_send_skill_allocation(self, body: str):
        try:
            data = json.loads(body)
            status, response = self.command_dispatch.queue_skill_allocation(data.get('allocation_key', ''))
            self.send_json_response(status, response)
        except json.JSONDecodeError as exc:
            self.send_json_response(400, {'error': str(exc)})

    def handle_send_task_graph(self, body: str):
        try:
            data = json.loads(body)
            status, response = self.command_dispatch.queue_task_graph(data.get('task_key', ''))
            self.send_json_response(status, response)
        except json.JSONDecodeError as exc:
            self.send_json_response(400, {'error': str(exc)})

    def handle_send_request_user_command(self):
        status, response = self.command_dispatch.queue_request_user_command()
        self.send_json_response(status, response)
