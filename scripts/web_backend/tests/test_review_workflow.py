from __future__ import annotations

import pathlib
import sys
import unittest


SCRIPTS_ROOT = pathlib.Path(__file__).resolve().parents[2]
if str(SCRIPTS_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_ROOT))

from web_backend.bootstrap.app_context import build_app_context


class ReviewWorkflowTest(unittest.TestCase):
    def test_task_graph_approval_queues_allocation_then_skill_list(self):
        app_context = build_app_context()

        status, response = app_context.command_dispatch.queue_task_graph('fire_rescue')
        self.assertEqual(status, 200)

        task_message = app_context.runtime.dequeue_hitl_message()
        self.assertIsNotNone(task_message)
        self.assertEqual(task_message['message_type'], 'task_graph')

        app_context.review_workflow.handle_review_response({
            'payload': {
                'original_message_id': task_message['message_id'],
                'approved': True,
            }
        })

        allocation_message = app_context.runtime.dequeue_hitl_message()
        self.assertIsNotNone(allocation_message)
        self.assertEqual(allocation_message['message_type'], 'skill_allocation')

        app_context.review_workflow.handle_review_response({
            'payload': {
                'original_message_id': allocation_message['message_id'],
                'approved': True,
            }
        })

        skill_message = app_context.runtime.dequeue_platform_message()
        self.assertIsNotNone(skill_message)
        self.assertEqual(skill_message['message_type'], 'skill_list')

    def test_rejected_review_stops_auto_progression(self):
        app_context = build_app_context()

        status, _response = app_context.command_dispatch.queue_task_graph('fire_rescue')
        self.assertEqual(status, 200)
        task_message = app_context.runtime.dequeue_hitl_message()
        self.assertIsNotNone(task_message)

        app_context.review_workflow.handle_review_response({
            'payload': {
                'original_message_id': task_message['message_id'],
                'approved': False,
            }
        })

        self.assertIsNone(app_context.runtime.dequeue_hitl_message())
        self.assertIsNone(app_context.runtime.dequeue_platform_message())

    def test_direct_skill_list_bypasses_hitl(self):
        app_context = build_app_context()

        status, response = app_context.command_dispatch.queue_skill_list('complete_test')

        self.assertEqual(status, 200)
        self.assertEqual(response['status'], 'queued')
        self.assertIsNone(app_context.runtime.dequeue_hitl_message())
        platform_message = app_context.runtime.dequeue_platform_message()
        self.assertIsNotNone(platform_message)
        self.assertEqual(platform_message['message_type'], 'skill_list')


if __name__ == '__main__':
    unittest.main()
