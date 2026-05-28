from __future__ import annotations

from dataclasses import dataclass

from web_backend.contexts.console.application.command_dispatch_service import CommandDispatchService


@dataclass(slots=True)
class DemoActionService:
    command_dispatch: CommandDispatchService

    def handle_demo_action(self, action_type: str, action_key: str) -> tuple[int, dict]:
        if action_type == 'skill_list':
            status, response = self.command_dispatch.queue_skill_list(action_key)
            if status == 200:
                response = {
                    'status': response['status'],
                    'action_type': action_type,
                    'action_key': action_key,
                    'message_id': response['message_id'],
                    'note': f"Demo: Skill list '{action_key}' queued for direct execution.",
                }
            return status, response

        if action_type == 'task_graph':
            status, response = self.command_dispatch.queue_task_graph(action_key)
            if status == 200:
                response = {
                    'status': response['status'],
                    'action_type': action_type,
                    'action_key': action_key,
                    'message_id': response['message_id'],
                    'note': f"Demo: Task graph '{action_key}' queued for HITL review.",
                }
            return status, response

        if action_type == 'skill_allocation':
            status, response = self.command_dispatch.queue_skill_allocation(action_key)
            if status == 200:
                response = {
                    'status': response['status'],
                    'action_type': action_type,
                    'action_key': action_key,
                    'message_id': response['message_id'],
                    'note': f"Demo: Skill allocation '{action_key}' queued for HITL review.",
                }
            return status, response

        return 400, {'error': f'Unknown action type: {action_type}'}
