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
    "Core/Manager/MASceneGraphManager.h",
    "Core/Manager/MASceneGraphNodeTypes.h",
    "Core/Types/MASceneGraphTypes.h",
    "Core/Manager/scene_graph_services/",
    "Core/Manager/scene_graph_ports/",
    "Core/Manager/scene_graph_adapters/",
    "Core/Manager/scene_graph_tools/",
    "Core/Manager/ue_tools/MAUESceneApplier.h",
    "Core/Manager/MAPIPCameraManager.h",
    "Core/Manager/MAExternalCameraManager.h",
    "Core/Manager/MAViewportManager.h",
    "Core/Manager/MAPIPViewExtension.h",
    "Core/Types/MAPIPCameraTypes.h",
    "Core/Manager/MAEditModeManager.h",
    "Core/Manager/MASelectionManager.h",
    "Core/Manager/MACommandManager.h",
)

FORBIDDEN_INFRA_FILES = (
    "Core/Interaction/Infrastructure/MAFeedbackPipeline.h",
    "Core/Interaction/Infrastructure/MAFeedbackPipeline.cpp",
)

REQUIRED_SCENEGRAPH_DIRS = (
    "Core/SceneGraph/Application",
    "Core/SceneGraph/Domain",
    "Core/SceneGraph/Infrastructure",
    "Core/SceneGraph/Runtime",
)

REQUIRED_CAMERA_DIRS = (
    "Core/Camera/Domain",
    "Core/Camera/Infrastructure",
    "Core/Camera/Runtime",
)

REQUIRED_EDITING_DIRS = (
    "Core/Editing/Runtime",
)

REQUIRED_SELECTION_DIRS = (
    "Core/Selection/Runtime",
)

REQUIRED_COMMAND_DIRS = (
    "Core/Command/Runtime",
)

FORBIDDEN_LEGACY_SCENEGRAPH_PATHS = (
    "Core/Manager/MASceneGraphManager.h",
    "Core/Manager/MASceneGraphManager.cpp",
    "Core/Manager/MASceneGraphNodeTypes.h",
    "Core/Types/MASceneGraphTypes.h",
    "Core/Manager/scene_graph_services",
    "Core/Manager/scene_graph_ports",
    "Core/Manager/scene_graph_adapters",
    "Core/Manager/scene_graph_tools",
    "Core/Manager/ue_tools/MAUESceneApplier.h",
    "Core/Manager/ue_tools/MAUESceneApplier.cpp",
)

FORBIDDEN_LEGACY_CAMERA_PATHS = (
    "Core/Manager/MAPIPCameraManager.h",
    "Core/Manager/MAPIPCameraManager.cpp",
    "Core/Manager/MAExternalCameraManager.h",
    "Core/Manager/MAExternalCameraManager.cpp",
    "Core/Manager/MAViewportManager.h",
    "Core/Manager/MAViewportManager.cpp",
    "Core/Manager/MAPIPViewExtension.h",
    "Core/Manager/MAPIPViewExtension.cpp",
    "Core/Types/MAPIPCameraTypes.h",
)

FORBIDDEN_LEGACY_EDITING_PATHS = (
    "Core/Manager/MAEditModeManager.h",
    "Core/Manager/MAEditModeManager.cpp",
)

FORBIDDEN_LEGACY_SELECTION_PATHS = (
    "Core/Manager/MASelectionManager.h",
    "Core/Manager/MASelectionManager.cpp",
)

FORBIDDEN_LEGACY_COMMAND_PATHS = (
    "Core/Manager/MACommandManager.h",
    "Core/Manager/MACommandManager.cpp",
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
        '#include "../Infrastructure/',
        '#include "../../Manager/',
        '#include "../Manager/',
        '#include "../../../Core/Manager/',
        '#include "../../../Agent/',
        '#include "../../../Environment/',
        '#include "../../Comm/MACommSubsystem.h"',
    ),
    "UI/HUD/Application/": (
        '#include "../Infrastructure/',
    ),
    "UI/Mode/Application/": (
        '#include "../Infrastructure/',
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

    for relative_path in REQUIRED_SCENEGRAPH_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required SceneGraph context directory: {relative_path}")

    for relative_path in REQUIRED_CAMERA_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required Camera context directory: {relative_path}")

    for relative_path in REQUIRED_EDITING_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required Editing context directory: {relative_path}")

    for relative_path in REQUIRED_SELECTION_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required Selection context directory: {relative_path}")

    for relative_path in REQUIRED_COMMAND_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required Command context directory: {relative_path}")

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

    for relative_path in FORBIDDEN_LEGACY_SCENEGRAPH_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after SceneGraph extraction")

    for relative_path in FORBIDDEN_LEGACY_CAMERA_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after Camera extraction")

    for relative_path in FORBIDDEN_LEGACY_EDITING_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after Editing extraction")

    for relative_path in FORBIDDEN_LEGACY_SELECTION_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after Selection extraction")

    for relative_path in FORBIDDEN_LEGACY_COMMAND_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after Command extraction")

    if errors:
        print("Interaction architecture guard failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("Interaction architecture guard passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
