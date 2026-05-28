from __future__ import annotations

import json
import queue
import threading
import time
from datetime import datetime

from web_backend.contexts.review_workflow.domain.review_models import PendingReview


class BackendRuntime:
    def __init__(self, ue_connection_timeout_seconds: float = 5.0):
        self.pending_platform_messages: list[dict] = []
        self.pending_hitl_messages: list[dict] = []
        self.platform_lock = threading.Lock()
        self.hitl_lock = threading.Lock()
        self.message_queue: queue.Queue[str] = queue.Queue()

        self.hitl_responses: list[dict] = []
        self.hitl_responses_lock = threading.Lock()

        self.pending_reviews: dict[str, PendingReview] = {}
        self.pending_reviews_lock = threading.Lock()

        self.ue_connection_timeout_seconds = ue_connection_timeout_seconds
        self.last_ue_poll_timestamp = 0.0
        self.ue_poll_lock = threading.Lock()

    def mark_ue_heartbeat(self) -> None:
        with self.ue_poll_lock:
            self.last_ue_poll_timestamp = time.time()

    def get_ue_connection_state(self) -> tuple[bool, float | None]:
        with self.ue_poll_lock:
            last_poll = self.last_ue_poll_timestamp

        if last_poll <= 0:
            return False, None

        age = time.time() - last_poll
        return age <= self.ue_connection_timeout_seconds, age

    def broadcast_message(self, msg_type: str, title: str, content, direction: str = 'incoming') -> None:
        payload = json.dumps({
            'type': msg_type,
            'title': title,
            'content': content,
            'direction': direction,
        }, ensure_ascii=False)
        self.message_queue.put(payload)

        timestamp = datetime.now().strftime('%H:%M:%S')
        arrow = '>>>' if direction == 'outgoing' else '<<<'
        print(f'[{timestamp}] {arrow} {title}')

    def enqueue_platform_message(self, message: dict) -> None:
        with self.platform_lock:
            self.pending_platform_messages.append(message)

    def dequeue_platform_message(self) -> dict | None:
        with self.platform_lock:
            if not self.pending_platform_messages:
                return None
            return self.pending_platform_messages.pop(0)

    def enqueue_hitl_message(self, message: dict) -> None:
        with self.hitl_lock:
            self.pending_hitl_messages.append(message)

    def dequeue_hitl_message(self) -> dict | None:
        with self.hitl_lock:
            if not self.pending_hitl_messages:
                return None
            return self.pending_hitl_messages.pop(0)

    def hitl_queue_size(self) -> int:
        with self.hitl_lock:
            return len(self.pending_hitl_messages)

    def append_hitl_response(self, response: dict) -> None:
        with self.hitl_responses_lock:
            self.hitl_responses.append(response)

    def register_pending_review(self, message_id: str, review_type: str, catalog_key: str) -> None:
        with self.pending_reviews_lock:
            self.pending_reviews[message_id] = PendingReview(review_type=review_type, catalog_key=catalog_key)

    def pop_pending_review(self, message_id: str) -> PendingReview | None:
        with self.pending_reviews_lock:
            return self.pending_reviews.pop(message_id, None)
