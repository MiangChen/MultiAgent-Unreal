from __future__ import annotations

import json

from web_backend.contexts.demo.domain.demo_scenarios import DEMO_SCENARIOS
from web_backend.contexts.demo.presentation.demo_body import get_demo_body_html
from web_backend.presentation.template_loader import load_text_template

HTML_TEMPLATE = load_text_template(__file__, 'demo_page.html')

def build_demo_page_html(*, skill_lists: dict, skill_allocations: dict) -> str:
    demo_content = get_demo_body_html()
    html = HTML_TEMPLATE.replace('{{DEMO_CONTENT}}', demo_content)
    html = html.replace('{{DEMO_SCENARIOS}}', json.dumps(DEMO_SCENARIOS, ensure_ascii=False))
    html = html.replace('{{SKILL_LISTS}}', json.dumps(skill_lists, ensure_ascii=False))
    html = html.replace('{{SKILL_ALLOCATIONS}}', json.dumps(skill_allocations, ensure_ascii=False))
    return html
