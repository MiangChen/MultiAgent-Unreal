#!/usr/bin/env python3
from __future__ import annotations

import ast
import pathlib
import sys


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPTS_ROOT = REPO_ROOT / 'scripts'
WEB_ROOT = SCRIPTS_ROOT / 'web_backend'

REQUIRED_DIRS = (
    WEB_ROOT / 'bootstrap',
    WEB_ROOT / 'presentation',
    WEB_ROOT / 'contexts' / 'catalog' / 'infrastructure',
    WEB_ROOT / 'contexts' / 'console' / 'application',
    WEB_ROOT / 'contexts' / 'console' / 'presentation',
    WEB_ROOT / 'contexts' / 'demo' / 'application',
    WEB_ROOT / 'contexts' / 'demo' / 'domain',
    WEB_ROOT / 'contexts' / 'demo' / 'presentation',
    WEB_ROOT / 'contexts' / 'messaging' / 'application',
    WEB_ROOT / 'contexts' / 'messaging' / 'infrastructure',
    WEB_ROOT / 'contexts' / 'review_workflow' / 'application',
    WEB_ROOT / 'contexts' / 'review_workflow' / 'domain',
)

PYTHON_FILES = (
    [SCRIPTS_ROOT / 'mock_backend.py', SCRIPTS_ROOT / 'demo_guided.py']
    + sorted(path for path in WEB_ROOT.rglob('*.py') if '__pycache__' not in path.parts)
)


def extract_imports(path: pathlib.Path) -> list[str]:
    tree = ast.parse(path.read_text(encoding='utf-8'), filename=str(path))
    imports: list[str] = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            imports.extend(alias.name for alias in node.names)
        elif isinstance(node, ast.ImportFrom):
            if node.module:
                imports.append(node.module)
    return imports


def classify_module(path: pathlib.Path) -> tuple[str, str | None]:
    if path == SCRIPTS_ROOT / 'mock_backend.py':
        return 'entrypoint', 'mock_backend'
    if path == SCRIPTS_ROOT / 'demo_guided.py':
        return 'entrypoint', 'demo_guided'

    relative = path.relative_to(WEB_ROOT)
    head = relative.parts[0]
    if head == 'bootstrap':
        return 'bootstrap', None
    if head == 'presentation':
        return 'presentation', None
    if head != 'contexts' or len(relative.parts) < 4:
        return 'unknown', None
    return relative.parts[2], relative.parts[1]


def import_layer(module_name: str) -> tuple[str, str | None]:
    if module_name == 'webui_asset_helpers':
        return 'asset_helper', None
    if not module_name.startswith('web_backend'):
        return 'external', None

    parts = module_name.split('.')
    if len(parts) < 2:
        return 'unknown', None
    if parts[1] == 'bootstrap':
        return 'bootstrap', None
    if parts[1] == 'presentation':
        return 'presentation', None
    if len(parts) >= 4 and parts[1] == 'contexts':
        return parts[3], parts[2]
    return 'unknown', None


def main() -> int:
    errors: list[str] = []

    for required_dir in REQUIRED_DIRS:
        if not required_dir.is_dir():
            errors.append(f'Missing required web backend directory: {required_dir.relative_to(REPO_ROOT)}')

    for path in PYTHON_FILES:
        file_layer, file_context = classify_module(path)
        imports = extract_imports(path)

        for module_name in imports:
            target_layer, target_context = import_layer(module_name)

            if target_layer in {'external', 'unknown'}:
                continue

            if file_layer == 'entrypoint':
                if path.name == 'mock_backend.py':
                    allowed = {
                        ('bootstrap', None),
                        ('presentation', None),
                    }
                    if (target_layer, target_context) not in allowed:
                        errors.append(
                            f'{path.relative_to(REPO_ROOT)} should only import bootstrap/presentation, found {module_name}'
                        )
                elif path.name == 'demo_guided.py':
                    allowed = {
                        ('application', 'demo'),
                        ('domain', 'demo'),
                        ('presentation', 'demo'),
                    }
                    if (target_layer, target_context) not in allowed:
                        errors.append(
                            f'{path.relative_to(REPO_ROOT)} should only import demo context modules, found {module_name}'
                        )
                continue

            if file_layer == 'bootstrap':
                if target_layer == 'presentation':
                    errors.append(
                        f'{path.relative_to(REPO_ROOT)} should not import presentation modules directly: {module_name}'
                    )
                continue

            if file_layer == 'presentation':
                if target_layer in {'bootstrap', 'infrastructure'}:
                    errors.append(
                        f'{path.relative_to(REPO_ROOT)} presentation must not import {target_layer}: {module_name}'
                    )
                continue

            if file_layer == 'domain':
                if target_layer in {'application', 'infrastructure', 'presentation', 'bootstrap'}:
                    errors.append(
                        f'{path.relative_to(REPO_ROOT)} domain must stay pure, found {module_name}'
                    )
                continue

            if file_layer == 'application':
                if target_layer in {'presentation', 'bootstrap'}:
                    errors.append(
                        f'{path.relative_to(REPO_ROOT)} application must not import {target_layer}: {module_name}'
                    )
                continue

            if file_layer == 'infrastructure':
                if target_layer in {'presentation', 'bootstrap'}:
                    errors.append(
                        f'{path.relative_to(REPO_ROOT)} infrastructure must not import {target_layer}: {module_name}'
                    )
                continue

    if errors:
        print('Web backend architecture check failed:')
        for error in errors:
            print(f' - {error}')
        return 1

    print('Web backend architecture check passed.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
