#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Mock Backend Server with Web UI.

Bootstrap entrypoint for the Python web backend.
"""

import argparse
import sys

from web_backend.bootstrap.app_context import build_app_context
from web_backend.presentation.http_handler import MockBackendHandler


def main() -> None:
    parser = argparse.ArgumentParser(description='MultiAgent-Unreal Mock Backend Server with Web UI')
    parser.add_argument('--port', type=int, default=8081, help='Server port (default: 8081, matches UE5 planner_url)')
    parser.add_argument('--host', type=str, default='0.0.0.0', help='Server address (default: 0.0.0.0)')
    args = parser.parse_args()

    server_address = (args.host, args.port)

    from http.server import ThreadingHTTPServer

    httpd = ThreadingHTTPServer(server_address, MockBackendHandler)
    httpd.app_context = build_app_context()  # type: ignore[attr-defined]

    print('=' * 60)
    print('  MultiAgent-Unreal Mock Backend Server')
    print('=' * 60)
    print(f'  🌐 Web UI:       http://localhost:{args.port}')
    print(f'  📡 Platform API: http://{args.host}:{args.port}/api/sim/poll')
    print(f'  🔄 HITL API:     http://{args.host}:{args.port}/api/hitl/poll')
    print('=' * 60)
    print('  Waiting for connections...')
    print('  Press Ctrl+C to stop')
    print('=' * 60)
    print()

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print('\nServer stopped')
        httpd.shutdown()
        sys.exit(0)


if __name__ == '__main__':
    main()
