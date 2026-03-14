from __future__ import annotations

from dataclasses import dataclass

from web_backend.contexts.catalog.infrastructure.static_catalog import StaticCatalog
from web_backend.contexts.messaging.application.message_factory import MessageFactory
from web_backend.contexts.messaging.infrastructure.runtime_state import BackendRuntime
from web_backend.contexts.review_workflow.application.review_workflow_service import ReviewWorkflowService


@dataclass(slots=True)
class CommandDispatchService:
    catalog: StaticCatalog
    runtime: BackendRuntime
    message_factory: MessageFactory
    review_workflow: ReviewWorkflowService

    def queue_skill_list(self, skill_key: str) -> tuple[int, dict]:
        skill_entry = self.catalog.get_skill_list(skill_key)
        if skill_entry is None:
            return 400, {'error': f'Unknown skill: {skill_key}'}

        message = self.message_factory.create_skill_list_message(skill_entry['data'])
        self.runtime.enqueue_platform_message(message)
        return 200, {
            'status': 'queued',
            'skill': skill_key,
            'message_id': message['message_id'],
            'note': 'Skill list queued. Will be executed directly by CommandManager.',
        }

    def queue_skill_allocation(self, allocation_key: str) -> tuple[int, dict]:
        allocation_entry = self.catalog.get_skill_allocation(allocation_key)
        if allocation_entry is None:
            return 400, {'error': f'Unknown skill allocation: {allocation_key}'}

        message = self.message_factory.create_skill_allocation_message(allocation_entry, category='review')
        self.runtime.enqueue_hitl_message(message)
        self.review_workflow.register_skill_allocation_review(message['message_id'], allocation_key)
        return 200, {
            'status': 'queued',
            'endpoint': '/api/hitl/poll',
            'allocation': allocation_key,
            'message_id': message['message_id'],
            'note': 'Skill allocation queued (HITL). Will show in Modal for user confirmation.',
        }

    def queue_task_graph(self, task_key: str) -> tuple[int, dict]:
        task_entry = self.catalog.get_task_graph(task_key)
        if task_entry is None:
            return 400, {'error': f'Unknown task graph: {task_key}'}

        message = self.message_factory.create_task_graph_message(task_entry['data'], category='review')
        self.runtime.enqueue_hitl_message(message)
        self.review_workflow.register_task_graph_review(message['message_id'], task_key)
        return 200, {
            'status': 'queued',
            'endpoint': '/api/hitl/poll',
            'task_graph': task_key,
            'message_id': message['message_id'],
            'note': 'Task graph queued (HITL). It will be displayed in Task Planner Widget.',
        }

    def queue_request_user_command(self) -> tuple[int, dict]:
        message = self.message_factory.create_request_user_command_message()
        self.runtime.enqueue_platform_message(message)
        self.runtime.broadcast_message('sent', 'user_instruction', message, 'outgoing')
        return 200, {
            'status': 'queued',
            'message_type': 'user_instruction',
            'message_id': message['message_id'],
            'note': 'Request user command message queued. UE5 will display a notification.',
        }
