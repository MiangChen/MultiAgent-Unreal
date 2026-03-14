from __future__ import annotations

from web_backend.presentation.template_loader import load_text_template


DEMO_HTML_CONTENT = load_text_template(__file__, 'demo_body.html')



def get_demo_body_html() -> str:
    return DEMO_HTML_CONTENT
