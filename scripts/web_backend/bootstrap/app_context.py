from __future__ import annotations

from dataclasses import dataclass

from web_backend.contexts.catalog.infrastructure.static_catalog import StaticCatalog, build_static_catalog
from web_backend.contexts.console.application.command_dispatch_service import CommandDispatchService
from web_backend.contexts.demo.application.demo_action_service import DemoActionService
from web_backend.contexts.messaging.application.message_factory import MessageFactory
from web_backend.contexts.messaging.infrastructure.runtime_state import BackendRuntime
from web_backend.contexts.review_workflow.application.review_workflow_service import ReviewWorkflowService


@dataclass(slots=True)
class MockBackendAppContext:
    catalog: StaticCatalog
    runtime: BackendRuntime
    message_factory: MessageFactory
    review_workflow: ReviewWorkflowService
    command_dispatch: CommandDispatchService
    demo_action: DemoActionService


def build_app_context() -> MockBackendAppContext:
    catalog = build_static_catalog()
    runtime = BackendRuntime()
    message_factory = MessageFactory()
    review_workflow = ReviewWorkflowService(
        catalog=catalog,
        runtime=runtime,
        message_factory=message_factory,
    )
    command_dispatch = CommandDispatchService(
        catalog=catalog,
        runtime=runtime,
        message_factory=message_factory,
        review_workflow=review_workflow,
    )
    demo_action = DemoActionService(command_dispatch=command_dispatch)
    return MockBackendAppContext(
        catalog=catalog,
        runtime=runtime,
        message_factory=message_factory,
        review_workflow=review_workflow,
        command_dispatch=command_dispatch,
        demo_action=demo_action,
    )
