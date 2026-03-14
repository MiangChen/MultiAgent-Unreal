from __future__ import annotations

import json

from web_backend.contexts.demo.domain.demo_scenarios import DEMO_SCENARIOS
from web_backend.contexts.demo.presentation.demo_body import get_demo_body_html


def build_demo_page_html(*, skill_lists: dict, skill_allocations: dict) -> str:
    demo_content = get_demo_body_html()
    return f'''<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MultiAgent - Guided Demo</title>
    <link rel="stylesheet" href="/assets/shared.css">
    <link rel="stylesheet" href="/assets/demo.css">
</head>
<body>
    <div class="top-tabs">
        <a class="top-tab" href="/">🎮 控制台</a>
        <a class="top-tab active demo-active" href="/demo">📖 引导演示 <span class="new-badge">NEW</span></a>
    </div>
    <div class="page">{demo_content}</div>
    <script>
        window.MA_DEMO_DATA = {{
            scenarios: {json.dumps(DEMO_SCENARIOS, ensure_ascii=False)},
            skillLists: {json.dumps(skill_lists, ensure_ascii=False)},
            skillAllocations: {json.dumps(skill_allocations, ensure_ascii=False)}
        }};
    </script>
    <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
    <script src="/assets/shared_ui.js"></script>
    <script src="/assets/demo.js"></script>
</body>
</html>'''
