from __future__ import annotations

from datetime import datetime
import uuid


class MessageFactory:
    def create_skill_list_message(self, skill_list: dict) -> dict:
        return {
            'message_type': 'skill_list',
            'message_category': 'platform',
            'timestamp': int(datetime.now().timestamp() * 1000),
            'message_id': str(uuid.uuid4()),
            'direction': 'outbound',
            'payload': skill_list,
        }

    def create_skill_allocation_message(self, allocation: dict, category: str = 'review') -> dict:
        return {
            'message_id': str(uuid.uuid4()),
            'message_category': category,
            'message_type': 'skill_allocation',
            'direction': 'python_to_ue5',
            'timestamp': datetime.now().isoformat(),
            'payload': {
                'review_type': 'skill_list',
                'data': allocation,
            },
        }

    def create_task_graph_message(self, task_graph: dict, category: str = 'review') -> dict:
        return {
            'message_id': str(uuid.uuid4()),
            'message_category': category,
            'message_type': 'task_graph',
            'direction': 'python_to_ue5',
            'timestamp': datetime.now().isoformat(),
            'payload': {
                'review_type': 'task_graph',
                'data': task_graph,
            },
        }

    def create_request_user_command_message(self) -> dict:
        return {
            'message_type': 'user_instruction',
            'timestamp': int(datetime.now().timestamp() * 1000),
            'message_id': str(uuid.uuid4()),
            'payload': {},
        }
