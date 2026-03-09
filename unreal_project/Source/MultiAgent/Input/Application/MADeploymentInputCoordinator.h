#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAInputTypes.h"

class AMAPlayerController;

class MULTIAGENT_API FMADeploymentInputCoordinator
{
public:
    void AddToQueue(AMAPlayerController* PlayerController, const FString& AgentType, int32 Count) const;
    void RemoveFromQueue(AMAPlayerController* PlayerController, const FString& AgentType, int32 Count) const;
    void ClearQueue(AMAPlayerController* PlayerController) const;

    int32 GetQueueCount(const AMAPlayerController* PlayerController) const;
    FString GetCurrentDeployingType(const AMAPlayerController* PlayerController) const;
    int32 GetCurrentDeployingCount(const AMAPlayerController* PlayerController) const;

    void EnterMode(AMAPlayerController* PlayerController) const;
    void EnterModeWithUnits(AMAPlayerController* PlayerController, const TArray<FMAPendingDeployment>& Deployments) const;
    void ExitMode(AMAPlayerController* PlayerController) const;

    void HandleLeftClickStarted(AMAPlayerController* PlayerController) const;
    void HandleLeftClickReleased(AMAPlayerController* PlayerController) const;
    void ResetTransientState(AMAPlayerController* PlayerController) const;

private:
    bool HasPendingDeployments(const AMAPlayerController* PlayerController) const;
    TArray<FVector> ProjectSelectionBoxToWorld(
        AMAPlayerController* PlayerController,
        const FVector2D& Start,
        const FVector2D& End,
        int32 Count) const;
    FVector ProjectToGround(AMAPlayerController* PlayerController, const FVector& WorldLocation) const;
};
