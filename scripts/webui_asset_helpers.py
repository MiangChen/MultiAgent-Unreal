from __future__ import annotations

import mimetypes
from pathlib import Path

ASSETS_DIR = Path(__file__).resolve().parent / 'webui_assets'


def resolve_asset_path(relative_path: str) -> Path | None:
    candidate = (ASSETS_DIR / relative_path.lstrip('/')).resolve()
    try:
        candidate.relative_to(ASSETS_DIR.resolve())
    except ValueError:
        return None
    if not candidate.is_file():
        return None
    return candidate


def read_asset_bytes(relative_path: str) -> tuple[bytes, str] | None:
    asset_path = resolve_asset_path(relative_path)
    if asset_path is None:
        return None
    content_type, _ = mimetypes.guess_type(asset_path.name)
    return asset_path.read_bytes(), (content_type or 'application/octet-stream')
