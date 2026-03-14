from __future__ import annotations

import json

from web_backend.presentation.template_loader import load_text_template


HTML_TEMPLATE = load_text_template(__file__, 'console_page.html')


def build_console_html(*, port: int, skill_lists: dict, task_graphs: dict, skill_allocations: dict) -> str:
    html = HTML_TEMPLATE.replace('{{PORT}}', str(port))
    html = html.replace('{{SKILL_LISTS}}', json.dumps(skill_lists, ensure_ascii=False))
    html = html.replace('{{TASK_GRAPHS}}', json.dumps(task_graphs, ensure_ascii=False))
    html = html.replace('{{SKILL_ALLOCATIONS}}', json.dumps(skill_allocations, ensure_ascii=False))
    return html
