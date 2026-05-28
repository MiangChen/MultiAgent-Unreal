#!/usr/bin/env python3
"""
MultiAgent-Unreal Interactive Demo Guide
=========================================

Thin entrypoint for the guided demo context.

- Module mode: re-exports demo scenarios, demo body HTML, and demo action handler.
- Standalone mode: runs the standalone demo server.
"""

from web_backend.contexts.demo.domain.demo_scenarios import DEMO_SCENARIOS
from web_backend.contexts.demo.presentation.demo_body import get_demo_body_html
from web_backend.contexts.demo.application.demo_action_service import DemoActionService
from web_backend.contexts.demo.presentation.standalone_server import main as standalone_main


def get_demo_html(_scenarios_json=None):
    return get_demo_body_html()


def handle_demo_action(action_type, action_key, app_context):
    service = DemoActionService(command_dispatch=app_context.command_dispatch)
    return service.handle_demo_action(action_type, action_key)


if __name__ == '__main__':
    standalone_main()
