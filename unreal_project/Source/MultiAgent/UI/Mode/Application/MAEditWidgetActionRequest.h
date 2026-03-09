#pragma once

#include "CoreMinimal.h"

class AMAPointOfInterest;

enum class EMAEditWidgetActionKind : uint8
{
    None,
    ApplyNodeEdit,
    DeleteActor,
    CreateGoal,
    CreateZone,
    AddPresetActor,
    DeletePOIs,
    SetGoalState
};

struct FMAEditWidgetActionRequest
{
    EMAEditWidgetActionKind Kind = EMAEditWidgetActionKind::None;
    AActor* Actor = nullptr;
    TArray<AMAPointOfInterest*> POIs;
    FString JsonContent;
    FString Description;
    FString ActorType;
    bool bGoalState = false;
};
