from __future__ import annotations

from pathlib import Path


def load_text_template(anchor_file: str, relative_path: str) -> str:
    return (Path(anchor_file).resolve().parent / relative_path).read_text(encoding='utf-8')
