#pragma once

#include "CoreMinimal.h"

class AMAHUD;
class UWorld;
class UMACommSubsystem;

class MULTIAGENT_API FMAHUDBackendRuntimeAdapter
{
public:
    UMACommSubsystem* ResolveCommSubsystem(UWorld* World) const;
    bool BindPlannerResponse(UWorld* World, AMAHUD* HUD) const;
    bool BindBackendEvents(UWorld* World, AMAHUD* HUD) const;
    bool SendNaturalLanguageCommand(UWorld* World, AMAHUD* HUD, const FString& Command) const;
    bool SendTaskGraphSubmit(UWorld* World, const FString& TaskGraphJson) const;
    bool SendReviewResponse(UWorld* World, bool bApproved, const FString& DataJson, const FString& RejectionReason) const;
    bool SendButtonEvent(UWorld* World, const FString& WidgetName, const FString& EventType, const FString& Label) const;
};
