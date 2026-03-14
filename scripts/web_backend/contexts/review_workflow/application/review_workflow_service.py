from __future__ import annotations

from web_backend.contexts.catalog.infrastructure.static_catalog import StaticCatalog
from web_backend.contexts.messaging.application.message_factory import MessageFactory
from web_backend.contexts.messaging.infrastructure.runtime_state import BackendRuntime


class ReviewWorkflowService:
    def __init__(self, catalog: StaticCatalog, runtime: BackendRuntime, message_factory: MessageFactory):
        self.catalog = catalog
        self.runtime = runtime
        self.message_factory = message_factory

    def register_task_graph_review(self, message_id: str, task_key: str) -> None:
        self.runtime.register_pending_review(message_id, 'task_graph', task_key)

    def register_skill_allocation_review(self, message_id: str, allocation_key: str) -> None:
        self.runtime.register_pending_review(message_id, 'skill_allocation', allocation_key)

    def handle_review_response(self, response: dict) -> None:
        payload = response.get('payload', {})
        original_message_id = payload.get('original_message_id')
        approved = payload.get('approved')

        if not original_message_id:
            return

        pending_review = self.runtime.pop_pending_review(original_message_id)
        if pending_review is None or approved is not True:
            return

        if pending_review.review_type == 'task_graph':
            self._queue_skill_allocation(pending_review.catalog_key)
            return

        if pending_review.review_type == 'skill_allocation':
            self._queue_skill_list(pending_review.catalog_key)

    def _queue_skill_allocation(self, task_key: str) -> None:
        allocation = self.catalog.get_skill_allocation(task_key)
        if allocation is None:
            return

        auto_message = self.message_factory.create_skill_allocation_message(allocation, category='review')
        self.runtime.enqueue_hitl_message(auto_message)
        self.register_skill_allocation_review(auto_message['message_id'], task_key)
        self.runtime.broadcast_message('sent', f'Auto Skill Allocation: {task_key}', auto_message, 'outgoing')
        print(f"[HITL Auto-Trigger] Automatically queued Skill Allocation for approved Task Graph '{task_key}'")

    def _queue_skill_list(self, allocation_key: str) -> None:
        skill_list_entry = self.catalog.get_skill_list(allocation_key)
        if skill_list_entry is None:
            return

        auto_message = self.message_factory.create_skill_list_message(skill_list_entry['data'])
        self.runtime.enqueue_platform_message(auto_message)
        self.runtime.broadcast_message('sent', f'Auto Execute Skill List: {allocation_key}', auto_message, 'outgoing')
        print(f"[HITL Auto-Trigger] Automatically queued Skill List execution for approved Skill Allocation '{allocation_key}'")
