#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE_ROOT = REPO_ROOT / "unreal_project" / "Source" / "MultiAgent"

OLD_INCLUDE_PATTERNS = (
    "Input/Application/",
    "Input/Domain/",
    "Input/Infrastructure/",
)

FORBIDDEN_INFRA_FILES = (
    "Core/Interaction/Infrastructure/MAFeedbackPipeline.h",
    "Core/Interaction/Infrastructure/MAFeedbackPipeline.cpp",
)

FORBIDDEN_DEBUG_PATTERN = "AddOnScreenDebugMessage"
ALLOWED_DEBUG_FILES = {
    "Core/Interaction/Infrastructure/MAFeedback21Applier.cpp",
}

FORBIDDEN_UI_MUTATION_PATTERNS = (
    "AppendStatusLog(",
    "LoadTaskGraphFromJson(",
)

ALLOWED_UI_MUTATION_FILES = {
    "Core/Interaction/Infrastructure/MAFeedback21Applier.cpp",
}

APPLICATION_RUNTIME_CALL_GUARDS = {
    "Core/Interaction/Application/": (
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
        "GetHitResultUnderCursor(",
        "LineTraceSingleByChannel(",
        "DeprojectScreenPositionToWorld(",
    ),
    "UI/HUD/Application/": (
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
    ),
    "UI/Mode/Application/": (
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
    ),
}

APPLICATION_RUNTIME_INCLUDE_GUARDS = {
    "Core/Interaction/Application/": (
        '#include "../../Manager/',
        '#include "../Manager/',
        '#include "../../../Core/Manager/',
        '#include "../../../Agent/',
        '#include "../../../Environment/',
        '#include "../../Comm/MACommSubsystem.h"',
    ),
    "UI/HUD/Application/": (
        '#include "../../../Core/Manager/',
        '#include "../../../Core/Comm/MACommSubsystem.h"',
        '#include "../../../Agent/',
        '#include "../../../Environment/',
    ),
}

DOMAIN_RUNTIME_TOKEN_GUARDS = {
    "Core/Interaction/Domain/": (
        "UWorld",
        "AActor",
        "UObject",
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
        "AMACharacter",
        "AMAPointOfInterest",
        "UMAAgentManager",
        "UMASelectionManager",
        "UMACommandManager",
        "UMASquadManager",
        "UMAEditModeManager",
    ),
    "UI/Mode/Domain/": (
        "UWorld",
        "AActor",
        "UObject",
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
        "AMACharacter",
        "AMAPointOfInterest",
        "UMAAgentManager",
        "UMASelectionManager",
        "UMACommandManager",
        "UMASquadManager",
        "UMAEditModeManager",
    ),
    "UI/HUD/Domain/": (
        "UWorld",
        "AActor",
        "UObject",
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
        "AMACharacter",
        "AMAPointOfInterest",
        "UMAAgentManager",
        "UMASelectionManager",
        "UMACommandManager",
        "UMASquadManager",
        "UMAEditModeManager",
    ),
}

LEGACY_STATE_GUARDS = {
    "Input/MAPlayerController.h": (
        "CurrentMouseMode =",
        "PreviousMouseMode =",
        "DeploymentQueue;",
        "CurrentDeploymentIndex =",
        "DeployedCount =",
        "HighlightedActors;",
    ),
}


def iter_source_files() -> list[pathlib.Path]:
    return [p for p in SOURCE_ROOT.rglob("*") if p.suffix in {".h", ".cpp"}]


def rel(path: pathlib.Path) -> str:
    return path.relative_to(SOURCE_ROOT).as_posix()


def main() -> int:
    errors: list[str] = []
    source_files = iter_source_files()

    feedback_dir = SOURCE_ROOT / "Core" / "Interaction" / "Feedback"
    required_feedback = {
        "MAFeedback21.h",
        "MAFeedback32.h",
        "MAFeedback43.h",
        "MAFeedback54.h",
    }
    existing_feedback = {p.name for p in feedback_dir.glob("*.h")}
    missing_feedback = sorted(required_feedback - existing_feedback)
    if missing_feedback:
        errors.append(f"Missing feedback headers: {', '.join(missing_feedback)}")

    for path in source_files:
        text = path.read_text(encoding="utf-8", errors="ignore")
        relative = rel(path)

        for pattern in OLD_INCLUDE_PATTERNS:
            if pattern in text:
                errors.append(f"{relative}: still references legacy interaction path '{pattern}'")

        debug_guard_scope = (
            relative.startswith("Core/Interaction/")
            or relative.startswith("UI/HUD/Application/")
            or relative == "Input/MAPlayerController.cpp"
        )
        if debug_guard_scope and FORBIDDEN_DEBUG_PATTERN in text and relative not in ALLOWED_DEBUG_FILES:
            errors.append(f"{relative}: direct screen debug output should go through Feedback21Applier")

        if relative.startswith("UI/HUD/Application/"):
            for pattern in FORBIDDEN_UI_MUTATION_PATTERNS:
                if pattern in text and relative not in ALLOWED_UI_MUTATION_FILES:
                    errors.append(f"{relative}: direct UI mutation '{pattern}' should go through FB21")

        for prefix, patterns in APPLICATION_RUNTIME_CALL_GUARDS.items():
            if relative.startswith(prefix):
                for pattern in patterns:
                    if pattern in text:
                        errors.append(f"{relative}: runtime call '{pattern}' should stay in Infrastructure/Bootstrap")

        for prefix, patterns in APPLICATION_RUNTIME_INCLUDE_GUARDS.items():
            if relative.startswith(prefix):
                for pattern in patterns:
                    if pattern in text:
                        errors.append(f"{relative}: runtime include '{pattern}' should stay in Infrastructure")

        for prefix, patterns in DOMAIN_RUNTIME_TOKEN_GUARDS.items():
            if relative.startswith(prefix):
                for pattern in patterns:
                    if pattern in text:
                        errors.append(f"{relative}: domain layer should not depend on runtime token '{pattern}'")

        for guarded_file, patterns in LEGACY_STATE_GUARDS.items():
            if relative == guarded_file:
                for pattern in patterns:
                    if pattern in text:
                        errors.append(f"{relative}: legacy controller state '{pattern}' should live in state objects")

    old_dirs = [
        SOURCE_ROOT / "Input" / "Application",
        SOURCE_ROOT / "Input" / "Domain",
        SOURCE_ROOT / "Input" / "Infrastructure",
    ]
    for directory in old_dirs:
        if directory.exists() and any(directory.iterdir()):
            errors.append(f"{directory.relative_to(REPO_ROOT)} should be empty after Interaction migration")

    for relative_path in FORBIDDEN_INFRA_FILES:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not live in Infrastructure; move pure feedback translation into Feedback/")

    if errors:
        print("Interaction architecture guard failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("Interaction architecture guard passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
