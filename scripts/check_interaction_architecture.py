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
    "UI/Mode/",
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
    "Core/Manager/MAAgentManager.h",
    "Core/Manager/MASquadManager.h",
    "Core/Manager/MATempDataManager.h",
    "Core/Manager/MAEnvironmentManager.h",
    "Core/MASquad.h",
    "Core/Types/MATypes.h",
    "Core/Types/MATaskGraphTypes.h",
    "Core/TaskGraph/Bootstrap/MATaskGraphBootstrap.h",
    "Core/TaskGraph/Bootstrap/MATaskGraphBootstrap.cpp",
    "Core/Types/MASimTypes.h",
    "Core/Shared/Types/MATaskGraphTypes.h",
    "Core/Shared/Types/MATaskGraphTypes.cpp",
    "Core/SkillAllocation/Bootstrap/MASkillAllocationBootstrap.h",
    "Core/SkillAllocation/Bootstrap/MASkillAllocationBootstrap.cpp",
    "Core/Comm/MACommSubsystem.h",
    "Core/Comm/MACommInbound.h",
    "Core/Comm/MACommOutbound.h",
    "Core/Comm/MACommHttpServer.h",
    "Core/Comm/MACommTypes.h",
    "Core/GameFlow/MAGameMode.h",
    "Core/GameFlow/MAGameInstance.h",
    "Core/GameFlow/MASetupGameMode.h",
    "UI/Modal/",
)

FORBIDDEN_INFRA_FILES = (
    "Core/Interaction/Infrastructure/MAFeedbackPipeline.h",
    "Core/Interaction/Infrastructure/MAFeedbackPipeline.cpp",
)

REQUIRED_SCENEGRAPH_DIRS = (
    "Core/SceneGraph/Application",
    "Core/SceneGraph/Bootstrap",
    "Core/SceneGraph/Domain",
    "Core/SceneGraph/Feedback",
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

REQUIRED_AGENT_RUNTIME_DIRS = (
    "Core/AgentRuntime/Runtime",
)

REQUIRED_SQUAD_DIRS = (
    "Core/Squad/Application",
    "Core/Squad/Bootstrap",
    "Core/Squad/Domain",
    "Core/Squad/Feedback",
    "Core/Squad/Infrastructure",
    "Core/Squad/Runtime",
)

REQUIRED_TEMPDATA_DIRS = (
    "Core/TempData/Runtime",
)

REQUIRED_ENVIRONMENT_CORE_DIRS = (
    "Core/EnvironmentCore/Runtime",
)

REQUIRED_SHARED_DIRS = (
    "Core/Shared/Types",
)

REQUIRED_TASKGRAPH_DIRS = (
    "Core/TaskGraph/Application",
    "Core/TaskGraph/Domain",
    "Core/TaskGraph/Feedback",
    "Core/TaskGraph/Infrastructure",
)

REQUIRED_UI_TASKGRAPH_DIRS = (
    "UI/TaskGraph/Application",
    "UI/TaskGraph/Bootstrap",
    "UI/TaskGraph/Domain",
    "UI/TaskGraph/Feedback",
    "UI/TaskGraph/Infrastructure",
    "UI/TaskGraph/Presentation",
)

REQUIRED_UI_SKILL_ALLOCATION_DIRS = (
    "UI/SkillAllocation/Application",
    "UI/SkillAllocation/Bootstrap",
    "UI/SkillAllocation/Domain",
    "UI/SkillAllocation/Feedback",
    "UI/SkillAllocation/Infrastructure",
    "UI/SkillAllocation/Presentation",
)

REQUIRED_UI_COMPONENTS_DIRS = (
    "UI/Components/Application",
    "UI/Components/Bootstrap",
    "UI/Components/Domain",
    "UI/Components/Infrastructure",
    "UI/Components/Presentation",
    "UI/Components/Runtime",
)

REQUIRED_UI_SETUP_DIRS = (
    "UI/Setup/Application",
    "UI/Setup/Bootstrap",
    "UI/Setup/Domain",
    "UI/Setup/Infrastructure",
    "UI/Setup/Presentation",
    "UI/Setup/Runtime",
)

REQUIRED_UI_CORE_MODAL_DIRS = (
    "UI/Core/Modal/Application",
    "UI/Core/Modal/Domain",
)

REQUIRED_UI_CORE_DIRS = (
    "UI/Core/Application",
    "UI/Core/Bootstrap",
    "UI/Core/Feedback",
    "UI/Core/Infrastructure",
)

REQUIRED_UI_HUD_DIRS = (
    "UI/HUD/Application",
    "UI/HUD/Bootstrap",
    "UI/HUD/Domain",
    "UI/HUD/Feedback",
    "UI/HUD/Infrastructure",
    "UI/HUD/Presentation",
    "UI/HUD/Runtime",
)

REQUIRED_UI_SCENE_EDITING_DIRS = (
    "UI/SceneEditing/Application",
    "UI/SceneEditing/Bootstrap",
    "UI/SceneEditing/Domain",
    "UI/SceneEditing/Feedback",
    "UI/SceneEditing/Infrastructure",
    "UI/SceneEditing/Presentation",
)

REQUIRED_SKILL_ALLOCATION_DIRS = (
    "Core/SkillAllocation/Application",
    "Core/SkillAllocation/Domain",
    "Core/SkillAllocation/Feedback",
    "Core/SkillAllocation/Infrastructure",
)

REQUIRED_COMM_DIRS = (
    "Core/Comm/Application",
    "Core/Comm/Bootstrap",
    "Core/Comm/Domain",
    "Core/Comm/Feedback",
    "Core/Comm/Infrastructure",
    "Core/Comm/Runtime",
)

REQUIRED_CONFIG_DIRS = (
    "Core/Config/Application",
    "Core/Config/Bootstrap",
    "Core/Config/Domain",
    "Core/Config/Infrastructure",
    "Core/Config/Runtime",
)

REQUIRED_GAMEFLOW_DIRS = (
    "Core/GameFlow/Bootstrap",
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

FORBIDDEN_LEGACY_AGENT_RUNTIME_PATHS = (
    "Core/Manager/MAAgentManager.h",
    "Core/Manager/MAAgentManager.cpp",
)

FORBIDDEN_LEGACY_SQUAD_PATHS = (
    "Core/Manager/MASquadManager.h",
    "Core/Manager/MASquadManager.cpp",
    "Core/MASquad.h",
    "Core/MASquad.cpp",
)

FORBIDDEN_LEGACY_TEMPDATA_PATHS = (
    "Core/Manager/MATempDataManager.h",
    "Core/Manager/MATempDataManager.cpp",
)

FORBIDDEN_LEGACY_ENVIRONMENT_CORE_PATHS = (
    "Core/Manager/MAEnvironmentManager.h",
    "Core/Manager/MAEnvironmentManager.cpp",
)

FORBIDDEN_LEGACY_SHARED_TYPE_PATHS = (
    "Core/Types/MATypes.h",
    "Core/Types/MATaskGraphTypes.h",
    "Core/Types/MATaskGraphTypes.cpp",
    "Core/Types/MASimTypes.h",
    "Core/Types/MASimTypes.cpp",
    "Core/Shared/Types/MATaskGraphTypes.h",
    "Core/Shared/Types/MATaskGraphTypes.cpp",
    "Core/TaskGraph/Infrastructure/MATaskGraphTypes.cpp",
)

FORBIDDEN_LEGACY_UI_MODAL_PATHS = (
    "UI/Modal",
)

FORBIDDEN_LEGACY_UI_SCENE_EDITING_PATHS = (
    "UI/Mode",
)

FORBIDDEN_LEGACY_UI_MISC_PATHS = (
    "UI/Legacy",
)

FORBIDDEN_LEGACY_COMM_PATHS = (
    "Core/Comm/MACommSubsystem.h",
    "Core/Comm/MACommSubsystem.cpp",
    "Core/Comm/MACommInbound.h",
    "Core/Comm/MACommInbound.cpp",
    "Core/Comm/MACommOutbound.h",
    "Core/Comm/MACommOutbound.cpp",
    "Core/Comm/MACommHttpServer.h",
    "Core/Comm/MACommHttpServer.cpp",
    "Core/Comm/MACommTypes.h",
)

FORBIDDEN_LEGACY_GAMEFLOW_PATHS = (
    "Core/GameFlow/MAGameMode.h",
    "Core/GameFlow/MAGameMode.cpp",
    "Core/GameFlow/MAGameInstance.h",
    "Core/GameFlow/MAGameInstance.cpp",
    "Core/GameFlow/MASetupGameMode.h",
    "Core/GameFlow/MASetupGameMode.cpp",
)

FORBIDDEN_LEGACY_CONFIG_PATHS = (
    "Core/Config/MAConfigManager.cpp",
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

SCENEGRAPH_SUBSYSTEM_PATTERN = "GetSubsystem<UMASceneGraphManager>()"
ALLOWED_SCENEGRAPH_SUBSYSTEM_FILES = {
    "Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.cpp",
}

SQUAD_SUBSYSTEM_PATTERN = "GetSubsystem<UMASquadManager>()"
ALLOWED_SQUAD_SUBSYSTEM_FILES = {
    "Core/Squad/Bootstrap/MASquadBootstrap.cpp",
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
    "UI/SceneEditing/Application/": (
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
    ),
    "UI/Core/Application/": (
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
    ),
    "UI/Setup/Application/": (
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
        "OpenLevel(",
        "SetInputMode(",
    ),
    "UI/Core/Modal/Application/": (
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
    "UI/SceneEditing/Application/": (
        '#include "../Infrastructure/',
    ),
    "UI/Core/Application/": (
        '#include "../../Infrastructure/',
        '#include "../../../Core/TempData/Runtime/',
        '#include "../../../Core/Command/Runtime/',
        '#include "../../../Core/Comm/Runtime/',
    ),
    "UI/Setup/Application/": (
        '#include "../Infrastructure/',
        '#include "../../../Core/Config/Runtime/',
        '#include "../../../Core/GameFlow/Bootstrap/',
        '#include "Kismet/GameplayStatics.h"',
    ),
    "UI/Core/Modal/Application/": (
        '#include "../Infrastructure/',
    ),
}

APPLICATION_BOOTSTRAP_INCLUDE_GUARDS = {
    "UI/Core/Application/": (
        '#include "../../TaskGraph/Bootstrap/',
        '#include "../../SkillAllocation/Bootstrap/',
        '#include "../../SceneEditing/Bootstrap/',
        '#include "../../Components/Bootstrap/',
    ),
    "UI/HUD/Application/": (
        '#include "../../Core/Bootstrap/',
        '#include "../Bootstrap/',
    ),
    "UI/SceneEditing/Application/": (
        '#include "../Bootstrap/',
    ),
    "UI/TaskGraph/Application/": (
        '#include "../Bootstrap/',
    ),
    "UI/SkillAllocation/Application/": (
        '#include "../Bootstrap/',
    ),
    "UI/Components/Application/": (
        '#include "../Bootstrap/',
    ),
    "UI/Setup/Application/": (
        '#include "../Bootstrap/',
    ),
}

ALLOWED_APPLICATION_RUNTIME_INCLUDE_FILES = {
    "UI/Setup/Application/MASetupHUDCoordinator.cpp",
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
    "UI/SceneEditing/Domain/": (
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
    "UI/Setup/Domain/": (
        "UWorld",
        "AActor",
        "UObject",
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
        "UGameplayStatics",
        "AHUD",
        "APlayerController",
        "UMAConfigManager",
    ),
    "UI/Core/Modal/Domain/": (
        "UWorld",
        "AActor",
        "UObject",
        "GetWorld(",
        "GetGameInstance(",
        "GetSubsystem<",
        "AHUD",
        "APlayerController",
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

TASKGRAPH_UI_RUNTIME_INCLUDE_GUARDS = (
    'Core/Comm/Runtime/',
    'Core/TempData/Runtime/',
    'Core/TaskGraph/Bootstrap/',
    'UI/Core/MAUIManager.h',
    'UI/HUD/Runtime/MAHUD.h',
)

TASKGRAPH_UI_RUNTIME_CALL_GUARDS = (
    "GetWorld(",
    "GetGameInstance(",
    "GetSubsystem<",
)

SKILLALLOCATION_UI_RUNTIME_INCLUDE_GUARDS = (
    'Core/Comm/Runtime/',
    'Core/TempData/Runtime/',
    'Core/SkillAllocation/Bootstrap/',
    'UI/Core/MAUIManager.h',
    'UI/HUD/Runtime/MAHUD.h',
)

SKILLALLOCATION_UI_RUNTIME_CALL_GUARDS = (
    "GetWorld(",
    "GetGameInstance(",
    "GetSubsystem<",
)

COMPONENTS_PREVIEW_RUNTIME_INCLUDE_GUARDS = (
    'Core/Comm/Runtime/',
    'Core/TempData/Runtime/',
    'UI/Core/MAUIManager.h',
    'UI/HUD/Runtime/MAHUD.h',
)

COMPONENTS_PREVIEW_RUNTIME_CALL_GUARDS = (
    "GetWorld(",
    "GetGameInstance(",
    "GetSubsystem<",
)

COMPONENTS_RUNTIME_WIDGET_SCOPE = {
    "UI/Components/Presentation/MAContextMenuWidget.h",
    "UI/Components/Presentation/MAContextMenuWidget.cpp",
    "UI/Components/Presentation/MAMiniMapWidget.h",
    "UI/Components/Presentation/MAMiniMapWidget.cpp",
    "UI/Components/Runtime/MAMiniMapManager.h",
    "UI/Components/Runtime/MAMiniMapManager.cpp",
    "UI/Components/Presentation/MANotificationWidget.h",
    "UI/Components/Presentation/MANotificationWidget.cpp",
    "UI/Components/Presentation/MAInstructionPanel.h",
    "UI/Components/Presentation/MAInstructionPanel.cpp",
    "UI/Components/Presentation/MASystemLogPanel.h",
    "UI/Components/Presentation/MASystemLogPanel.cpp",
    "UI/Components/Presentation/MADirectControlIndicator.h",
    "UI/Components/Presentation/MADirectControlIndicator.cpp",
    "UI/Components/Presentation/MASpeechBubbleWidget.h",
    "UI/Components/Presentation/MASpeechBubbleWidget.cpp",
    "UI/Components/Presentation/MAStyledButton.h",
    "UI/Components/Presentation/MAStyledButton.cpp",
}

COMPONENTS_RUNTIME_INCLUDE_GUARDS = (
    'Core/AgentRuntime/Runtime/',
    'Core/Selection/Runtime/',
    'Core/Command/Runtime/',
    'Core/TempData/Runtime/',
    'Core/Camera/Runtime/',
    'UI/Core/MAUIManager.h',
    'UI/HUD/Runtime/MAHUD.h',
)

COMPONENTS_RUNTIME_CALL_GUARDS = (
    "GetWorld(",
    "GetGameInstance(",
    "GetSubsystem<",
    "UGameplayStatics::GetPlayerController(",
)

SETUP_RUNTIME_WIDGET_SCOPE = {
    "UI/Setup/Runtime/MASetupHUD.h",
    "UI/Setup/Runtime/MASetupHUD.cpp",
    "UI/Setup/Presentation/MASetupWidget.h",
    "UI/Setup/Presentation/MASetupWidget.cpp",
}

SETUP_RUNTIME_INCLUDE_GUARDS = (
    'Core/Config/MAConfigManager.h',
    'Core/Config/Runtime/',
    'Core/GameFlow/Bootstrap/',
    'Kismet/GameplayStatics.h',
)

SETUP_RUNTIME_CALL_GUARDS = (
    "GetWorld(",
    "GetGameInstance(",
    "GetSubsystem<",
    "OpenLevel(",
    "SetInputMode(",
)

MODE_RUNTIME_WIDGET_SCOPE = {
    "UI/SceneEditing/Presentation/MAModifyWidget.h",
    "UI/SceneEditing/Presentation/MAModifyWidget.cpp",
    "UI/SceneEditing/Presentation/MASceneListWidget.h",
    "UI/SceneEditing/Presentation/MASceneListWidget.cpp",
}

MODE_RUNTIME_INCLUDE_GUARDS = (
    'Core/Editing/Runtime/',
    'Core/SceneGraph/Runtime/',
    'Core/SceneGraph/Bootstrap/',
    'Core/Comm/Runtime/',
)

MODE_RUNTIME_CALL_GUARDS = (
    "GetWorld(",
    "GetGameInstance(",
    "GetSubsystem<",
)

MODAL_RUNTIME_WIDGET_SCOPE = {
    "UI/Core/Modal/MABaseModalWidget.h",
    "UI/Core/Modal/MABaseModalWidget.cpp",
    "UI/Core/Modal/MADecisionModal.h",
    "UI/Core/Modal/MADecisionModal.cpp",
}

MODAL_RUNTIME_INCLUDE_GUARDS = (
    '#include "../../Core/Comm/Runtime/',
    '#include "../../Core/TempData/Runtime/',
)

MODAL_RUNTIME_CALL_GUARDS = (
    "GetWorld(",
    "GetGameInstance(",
    "GetSubsystem<",
)

SELECTIONHUD_RUNTIME_INCLUDE_GUARDS = (
    'Core/Selection/Runtime/',
    'Core/AgentRuntime/Runtime/',
    'UI/Core/MAUIManager.h',
    'Blueprint/UserWidget.h',
    'Blueprint/WidgetBlueprintLibrary.h',
)

SELECTIONHUD_RUNTIME_CALL_GUARDS = (
    "GetSubsystem<UMASelectionManager>()",
    "GetSubsystem<UMAAgentManager>()",
    "GetAllWidgetsOfClass(",
)


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

    for relative_path in REQUIRED_AGENT_RUNTIME_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required AgentRuntime context directory: {relative_path}")

    for relative_path in REQUIRED_SQUAD_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required Squad context directory: {relative_path}")

    for relative_path in REQUIRED_TEMPDATA_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required TempData context directory: {relative_path}")

    for relative_path in REQUIRED_ENVIRONMENT_CORE_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required EnvironmentCore context directory: {relative_path}")

    for relative_path in REQUIRED_SHARED_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required Shared context directory: {relative_path}")

    for relative_path in REQUIRED_TASKGRAPH_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required TaskGraph context directory: {relative_path}")

    for relative_path in REQUIRED_UI_TASKGRAPH_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required UI TaskGraph directory: {relative_path}")

    for relative_path in REQUIRED_UI_SKILL_ALLOCATION_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required UI SkillAllocation directory: {relative_path}")

    for relative_path in REQUIRED_UI_COMPONENTS_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required UI Components directory: {relative_path}")

    for relative_path in REQUIRED_UI_SETUP_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required UI Setup directory: {relative_path}")

    for relative_path in REQUIRED_UI_CORE_MODAL_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required UI Core Modal directory: {relative_path}")

    for relative_path in REQUIRED_UI_CORE_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required UI Core directory: {relative_path}")

    for relative_path in REQUIRED_UI_HUD_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required UI HUD directory: {relative_path}")

    for relative_path in REQUIRED_UI_SCENE_EDITING_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required UI SceneEditing directory: {relative_path}")

    for relative_path in REQUIRED_SKILL_ALLOCATION_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required SkillAllocation context directory: {relative_path}")

    for relative_path in REQUIRED_COMM_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required Comm context directory: {relative_path}")

    for relative_path in REQUIRED_CONFIG_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required Config context directory: {relative_path}")

    for relative_path in REQUIRED_GAMEFLOW_DIRS:
        if not (SOURCE_ROOT / relative_path).exists():
            errors.append(f"Missing required GameFlow bootstrap directory: {relative_path}")

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

        if SCENEGRAPH_SUBSYSTEM_PATTERN in text and relative not in ALLOWED_SCENEGRAPH_SUBSYSTEM_FILES:
            errors.append(
                f"{relative}: SceneGraph subsystem lookup should go through Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.cpp"
            )

        if SQUAD_SUBSYSTEM_PATTERN in text and relative not in ALLOWED_SQUAD_SUBSYSTEM_FILES:
            errors.append(
                f"{relative}: Squad subsystem lookup should go through Core/Squad/Bootstrap/MASquadBootstrap.cpp"
            )

        for prefix, patterns in APPLICATION_RUNTIME_CALL_GUARDS.items():
            if relative.startswith(prefix):
                for pattern in patterns:
                    if pattern in text:
                        errors.append(f"{relative}: runtime call '{pattern}' should stay in Infrastructure/Bootstrap")

        for prefix, patterns in APPLICATION_RUNTIME_INCLUDE_GUARDS.items():
            if relative.startswith(prefix):
                for pattern in patterns:
                    if pattern in text and relative not in ALLOWED_APPLICATION_RUNTIME_INCLUDE_FILES:
                        errors.append(f"{relative}: runtime include '{pattern}' should stay in Infrastructure")

        for prefix, patterns in APPLICATION_BOOTSTRAP_INCLUDE_GUARDS.items():
            if relative.startswith(prefix):
                for pattern in patterns:
                    if pattern in text:
                        errors.append(f"{relative}: bootstrap include '{pattern}' should stay in Bootstrap or entry shell")

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

        if relative.startswith("UI/TaskGraph/") and not relative.startswith("UI/TaskGraph/Infrastructure/"):
            for pattern in TASKGRAPH_UI_RUNTIME_INCLUDE_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime include '{pattern}' should stay in UI/TaskGraph/Infrastructure")

            for pattern in TASKGRAPH_UI_RUNTIME_CALL_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime call '{pattern}' should stay in UI/TaskGraph/Infrastructure")

        if relative.startswith("UI/SkillAllocation/") and not relative.startswith("UI/SkillAllocation/Infrastructure/"):
            for pattern in SKILLALLOCATION_UI_RUNTIME_INCLUDE_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime include '{pattern}' should stay in UI/SkillAllocation/Infrastructure")

            for pattern in SKILLALLOCATION_UI_RUNTIME_CALL_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime call '{pattern}' should stay in UI/SkillAllocation/Infrastructure")

        preview_component_scope = (
            relative in {
                "UI/Components/Presentation/MAPreviewPanel.h",
                "UI/Components/Presentation/MAPreviewPanel.cpp",
                "UI/Components/Presentation/MATaskGraphPreview.h",
                "UI/Components/Presentation/MATaskGraphPreview.cpp",
                "UI/Components/Presentation/MASkillListPreview.h",
                "UI/Components/Presentation/MASkillListPreview.cpp",
            }
            or relative.startswith("UI/Components/Application/")
            or relative.startswith("UI/Components/Domain/")
            or relative.startswith("UI/Components/Infrastructure/")
        )
        if preview_component_scope and not relative.startswith("UI/Components/Infrastructure/"):
            for pattern in COMPONENTS_PREVIEW_RUNTIME_INCLUDE_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime include '{pattern}' should stay in UI/Components/Infrastructure")

            for pattern in COMPONENTS_PREVIEW_RUNTIME_CALL_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime call '{pattern}' should stay in UI/Components/Infrastructure")

        runtime_component_scope = (
            relative in COMPONENTS_RUNTIME_WIDGET_SCOPE
            or relative.startswith("UI/Components/Application/")
            or relative.startswith("UI/Components/Domain/")
        )
        if runtime_component_scope:
            for pattern in COMPONENTS_RUNTIME_INCLUDE_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime include '{pattern}' should stay in UI/Components/Infrastructure")

            for pattern in COMPONENTS_RUNTIME_CALL_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime call '{pattern}' should stay in UI/Components/Infrastructure")

        if relative in SETUP_RUNTIME_WIDGET_SCOPE:
            for pattern in SETUP_RUNTIME_INCLUDE_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime include '{pattern}' should stay in UI/Setup/Infrastructure")

            for pattern in SETUP_RUNTIME_CALL_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime call '{pattern}' should stay in UI/Setup/Infrastructure")

        if relative in MODE_RUNTIME_WIDGET_SCOPE:
            for pattern in MODE_RUNTIME_INCLUDE_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime include '{pattern}' should stay in UI/SceneEditing/Infrastructure")

            for pattern in MODE_RUNTIME_CALL_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime call '{pattern}' should stay in UI/SceneEditing/Infrastructure")

        if relative in MODAL_RUNTIME_WIDGET_SCOPE:
            for pattern in MODAL_RUNTIME_INCLUDE_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime include '{pattern}' should stay out of UI/Core/Modal widget shell")

            for pattern in MODAL_RUNTIME_CALL_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime call '{pattern}' should stay out of UI/Core/Modal widget shell")

        if relative == "UI/HUD/Runtime/MASelectionHUD.cpp":
            for pattern in SELECTIONHUD_RUNTIME_INCLUDE_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime include '{pattern}' should stay in UI/HUD/Infrastructure")

            for pattern in SELECTIONHUD_RUNTIME_CALL_GUARDS:
                if pattern in text:
                    errors.append(f"{relative}: runtime call '{pattern}' should stay in UI/HUD/Infrastructure")

    old_dirs = [
        SOURCE_ROOT / "Input" / "Application",
        SOURCE_ROOT / "Input" / "Domain",
        SOURCE_ROOT / "Input" / "Infrastructure",
    ]
    for directory in old_dirs:
        if directory.exists() and any(directory.iterdir()):
            errors.append(f"{directory.relative_to(REPO_ROOT)} should be empty after Interaction migration")

    legacy_manager_dir = SOURCE_ROOT / "Core" / "Manager"
    if legacy_manager_dir.exists():
        remaining_entries = [p.name for p in legacy_manager_dir.iterdir() if p.name != ".DS_Store"]
        if remaining_entries:
            errors.append(
                f"{legacy_manager_dir.relative_to(REPO_ROOT)} should be retired after context extraction; found: {', '.join(sorted(remaining_entries))}"
            )

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

    for relative_path in FORBIDDEN_LEGACY_AGENT_RUNTIME_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after AgentRuntime extraction")

    for relative_path in FORBIDDEN_LEGACY_SQUAD_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after Squad extraction")

    for relative_path in FORBIDDEN_LEGACY_TEMPDATA_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after TempData extraction")

    for relative_path in FORBIDDEN_LEGACY_ENVIRONMENT_CORE_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after EnvironmentCore extraction")

    for relative_path in FORBIDDEN_LEGACY_SHARED_TYPE_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after Shared types extraction")

    for relative_path in FORBIDDEN_LEGACY_UI_MODAL_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after modal merge into UI/Core and owner contexts")

    for relative_path in FORBIDDEN_LEGACY_UI_SCENE_EDITING_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after SceneEditing rename")

    for relative_path in FORBIDDEN_LEGACY_UI_MISC_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after UI context consolidation")

    for relative_path in FORBIDDEN_LEGACY_COMM_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after Comm context layering")

    for relative_path in FORBIDDEN_LEGACY_GAMEFLOW_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after GameFlow bootstrap consolidation")

    for relative_path in FORBIDDEN_LEGACY_CONFIG_PATHS:
        if (SOURCE_ROOT / relative_path).exists():
            errors.append(f"{relative_path} should not exist after Config context layering")

    if errors:
        print("Interaction architecture guard failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("Interaction architecture guard passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
