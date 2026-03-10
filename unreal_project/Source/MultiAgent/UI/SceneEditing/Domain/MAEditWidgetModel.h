#pragma once

#include "CoreMinimal.h"
#include "../../../Core/SceneGraph/Domain/MASceneGraphNodeTypes.h"

enum class EMAEditWidgetHintTone : uint8
{
    Default,
    Info,
    Success,
    Warning,
    Error
};

struct FMAEditWidgetNodeTabViewModel
{
    FString Label;
    bool bSelected = false;
};

struct FMAEditWidgetViewModel
{
    FString HintText = TEXT("Select an Actor or POI to operate");
    EMAEditWidgetHintTone HintTone = EMAEditWidgetHintTone::Default;
    FString ErrorText;
    FString JsonContent;
    bool bJsonReadOnly = true;

    bool bShowActorOperations = false;
    bool bShowPOIOperations = false;
    bool bShowCreateGoal = false;
    bool bShowCreateZone = false;
    bool bShowPresetActorControls = false;
    bool bShowDeleteActor = false;
    bool bShowSetAsGoal = false;
    bool bShowUnsetAsGoal = false;

    TArray<FMAEditWidgetNodeTabViewModel> NodeTabs;
    int32 CurrentNodeIndex = 0;
};
