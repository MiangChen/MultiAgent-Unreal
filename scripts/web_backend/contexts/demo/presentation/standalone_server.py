from __future__ import annotations

import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.error import URLError
from urllib.parse import urlparse
from urllib.request import Request, urlopen

from web_backend.contexts.demo.domain.demo_scenarios import DEMO_SCENARIOS
from web_backend.contexts.demo.presentation.demo_body import get_demo_body_html
from webui_asset_helpers import read_asset_bytes


STANDALONE_HTML = '''<!DOCTYPE html>
<html lang="zh-CN"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>MultiAgent Interactive Demo</title>
<link rel="stylesheet" href="/assets/shared.css">
<link rel="stylesheet" href="/assets/demo.css">
<style>
    .sa-header { height: 50px; background: var(--panel-bg); border-bottom: 1px solid var(--border);
                 display: flex; align-items: center; padding: 0 24px; gap: 12px; }
    .sa-title { font-size: 16px; font-weight: 700; color: var(--amber); }
    .sa-status { margin-left: auto; font-size: 12px; color: var(--muted); }
    .sa-dot { width: 8px; height: 8px; border-radius: 50%; background: #f44; display: inline-block; margin-right: 6px; }
    .sa-dot.ok { background: var(--green); }
    .sa-wrap { height: calc(100vh - 50px); overflow: hidden; }
</style></head><body>
<div class="sa-header">
    <span>🎮</span> <span class="sa-title">MultiAgent Interactive Demo (Standalone)</span>
    <div class="sa-status"><span class="sa-dot" id="sa-dot"></span><span id="sa-st">...</span></div>
</div>
<div class="sa-wrap">''' + get_demo_body_html() + '''</div>
<script>
window.MA_DEMO_DATA = {
    scenarios: {{DEMO_SCENARIOS}},
    skillLists: {},
    skillAllocations: {}
};
</script>
<script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
<script src="/assets/shared_ui.js"></script>
<script src="/assets/demo.js"></script>
<script>
async function checkBk(){try{const r=await fetch('/api/health_proxy');const d=await r.json();
const ok=d.status==='healthy';document.getElementById('sa-dot').className='sa-dot'+(ok?' ok':'');
document.getElementById('sa-st').textContent=ok?'Backend OK':'Error';}catch(e){
document.getElementById('sa-dot').className='sa-dot';document.getElementById('sa-st').textContent='Offline';}}
checkBk();setInterval(checkBk,3000);
</script></body></html>'''


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
                html = STANDALONE_HTML.replace('{{DEMO_SCENARIOS}}', json.dumps(DEMO_SCENARIOS, ensure_ascii=False))
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
