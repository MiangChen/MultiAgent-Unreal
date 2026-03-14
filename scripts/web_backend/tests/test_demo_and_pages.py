from __future__ import annotations

import pathlib
import sys
import unittest


SCRIPTS_ROOT = pathlib.Path(__file__).resolve().parents[2]
if str(SCRIPTS_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_ROOT))

from web_backend.bootstrap.app_context import build_app_context
from web_backend.contexts.console.presentation.console_page import build_console_html
from web_backend.contexts.demo.presentation.demo_page import build_demo_page_html


class DemoAndPagesTest(unittest.TestCase):
    def test_demo_action_dispatches_known_step(self):
        app_context = build_app_context()

        status, response = app_context.demo_action.handle_demo_action('task_graph', 'fire_rescue')

        self.assertEqual(status, 200)
        self.assertEqual(response['action_type'], 'task_graph')
        self.assertEqual(response['action_key'], 'fire_rescue')
        queued = app_context.runtime.dequeue_hitl_message()
        self.assertIsNotNone(queued)
        self.assertEqual(queued['message_type'], 'task_graph')

    def test_demo_action_rejects_unknown_type(self):
        app_context = build_app_context()

        status, response = app_context.demo_action.handle_demo_action('unknown', 'fire_rescue')

        self.assertEqual(status, 400)
        self.assertIn('Unknown action type', response['error'])

    def test_console_html_contains_four_entry_groups(self):
        app_context = build_app_context()

        html = build_console_html(
            port=8081,
            skill_lists=app_context.catalog.skill_lists,
            task_graphs=app_context.catalog.task_graphs,
            skill_allocations=app_context.catalog.skill_allocations,
        )

        self.assertIn('待审核计划', html)
        self.assertIn('待审核排班', html)
        self.assertIn('直接执行示例', html)
        self.assertIn('完整流程 Demo', html)
        self.assertIn('window.MA_CONSOLE_DATA', html)

    def test_demo_html_contains_serialized_scenarios(self):
        app_context = build_app_context()

        html = build_demo_page_html(
            skill_lists=app_context.catalog.skill_lists,
            skill_allocations=app_context.catalog.skill_allocations,
        )

        self.assertIn('window.MA_DEMO_DATA', html)
        self.assertIn('fire_rescue', html)
        self.assertIn('full_collaboration', html)


if __name__ == '__main__':
    unittest.main()
