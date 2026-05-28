from __future__ import annotations

import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.error import URLError
from urllib.parse import urlparse
from urllib.request import Request, urlopen

from web_backend.contexts.demo.domain.demo_scenarios import DEMO_SCENARIOS
from web_backend.contexts.demo.presentation.demo_body import get_demo_body_html
from web_backend.presentation.template_loader import load_text_template
from webui_asset_helpers import read_asset_bytes


STANDALONE_TEMPLATE = load_text_template(__file__, 'standalone_page.html')


def run_standalone_demo_server(port: int, backend_url: str) -> None:
    backend_base_url = backend_url.rstrip('/')

    def proxy_to_backend(path: str, method: str = 'GET', body: str | None = None):
        url = backend_base_url + path
        req = Request(url, method=method)
        if body:
            req.data = body.encode('utf-8')
            req.add_header('Content-Type', 'application/json')
        try:
            with urlopen(req, timeout=5) as resp:
                return resp.status, resp.read().decode('utf-8')
        except URLError as exc:
            return 502, json.dumps({'error': f'Backend unreachable: {exc}'})
        except Exception as exc:
            return 500, json.dumps({'error': str(exc)})

    class StandaloneDemoHandler(BaseHTTPRequestHandler):
        def log_message(self, fmt, *args):
            pass

        def send_json(self, code, data):
            self.send_response(code)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(data, ensure_ascii=False).encode('utf-8'))

        def do_GET(self):
            path = urlparse(self.path).path
            if path in ('/', '/index.html'):
                html = STANDALONE_TEMPLATE.replace('{{DEMO_CONTENT}}', get_demo_body_html())
                html = html.replace('{{DEMO_SCENARIOS}}', json.dumps(DEMO_SCENARIOS, ensure_ascii=False))
                self.send_response(200)
                self.send_header('Content-Type', 'text/html; charset=utf-8')
                self.end_headers()
                self.wfile.write(html.encode('utf-8'))
            elif path.startswith('/assets/'):
                asset = read_asset_bytes(path[len('/assets/'):])
                if asset is None:
                    self.send_response(404)
                    self.end_headers()
                    return
                body, content_type = asset
                self.send_response(200)
                self.send_header('Content-Type', content_type)
                self.end_headers()
                self.wfile.write(body)
            elif path == '/api/health_proxy':
                code, body = proxy_to_backend('/api/health')
                self.send_response(code)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(body.encode('utf-8'))
            else:
                self.send_response(404)
                self.end_headers()

        def do_POST(self):
            path = urlparse(self.path).path
            length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(length).decode('utf-8') if length > 0 else ''
            if path not in ('/api/proxy', '/api/demo/send_step'):
                self.send_response(404)
                self.end_headers()
                return

            try:
                data = json.loads(body)
                action_type = data.get('action_type', '')
                action_key = data.get('action_key', '')
                api_map = {
                    'skill_list': '/api/send_skill',
                    'task_graph': '/api/send_task_graph',
                    'skill_allocation': '/api/send_skill_allocation',
                }
                key_map = {
                    'skill_list': 'skill_key',
                    'task_graph': 'task_key',
                    'skill_allocation': 'allocation_key',
                }
                if action_type not in api_map:
                    self.send_json(400, {'error': f'Unknown action type: {action_type}'})
                    return

                code, response_body = proxy_to_backend(api_map[action_type], 'POST', json.dumps({key_map[action_type]: action_key}))
                try:
                    response_data = json.loads(response_body)
                except json.JSONDecodeError:
                    response_data = {'raw': response_body}
                self.send_json(code, response_data)
            except json.JSONDecodeError as exc:
                self.send_json(400, {'error': str(exc)})

    server = ThreadingHTTPServer(('0.0.0.0', port), StandaloneDemoHandler)
    print(f'Demo (standalone): http://localhost:{port}  |  Backend: {backend_base_url}')
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print('\nStopped')
        server.shutdown()


def main() -> None:
    parser = argparse.ArgumentParser(description='MultiAgent Demo Guide (Standalone)')
    parser.add_argument('--port', type=int, default=8082)
    parser.add_argument('--backend', type=str, default='http://localhost:8081')
    args = parser.parse_args()
    run_standalone_demo_server(port=args.port, backend_url=args.backend)
