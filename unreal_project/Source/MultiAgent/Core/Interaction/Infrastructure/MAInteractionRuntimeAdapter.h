#pragma once

#include "CoreMinimal.h"
#include "Core/Command/Domain/MACommandTypes.h"
#include "../Domain/MAInteractionRuntimeTypes.h"
#include "../Feedback/MAFeedback54.h"

class AActor;
class AMACharacter;
class AMAPointOfInterest;
class AMAPlayerController;

class UMACameraSensorComponent;
struct FMASquadOperationFeedback;

enum class EMAAgentType : uint8;

class MULTIAGENT_API FMAInteractionRuntimeAdapter
{
public:
    bool IsSelectionBoxActive(const AMAPlayerController* PlayerController) const;
    void BeginSelectionBox(AMAPlayerController* PlayerController, const FVector2D& Start) const;
    void UpdateSelectionBox(AMAPlayerController* PlayerController, const FVector2D& Current) const;
    void EndSelectionBox(AMAPlayerController* PlayerController, bool bAppendSelection) const;
    void CancelSelectionBox(AMAPlayerController* PlayerController) const;
    FVector2D GetSelectionBoxStart(const AMAPlayerController* PlayerController) const;
    FVector2D GetSelectionBoxEnd(const AMAPlayerController* PlayerController) const;

    bool ResolveCursorHit(
        AMAPlayerController* PlayerController,
        ECollisionChannel CollisionChannel,
        FHitResult& OutHitResult) const;
    bool ResolveMouseHitLocation(AMAPlayerController* PlayerController, FVector& OutLocation) const;
    TArray<FVector> ProjectSelectionBoxToWorld(
        AMAPlayerController* PlayerController,
        const FVector2D& Start,
        const FVector2D& End,
        int32 Count) const;

    AMACharacter* ResolveClickedAgent(AMAPlayerController* PlayerController) const;
    void ToggleSelection(AMAPlayerController* PlayerController, AMACharacter* Agent) const;
    void SelectAgent(AMAPlayerController* PlayerController, AMACharacter* Agent) const;
    TArray<AMACharacter*> GetSelectedAgents(const AMAPlayerController* PlayerController) const;
    void SaveToControlGroup(AMAPlayerController* PlayerController, int32 GroupIndex) const;
    void SelectControlGroup(AMAPlayerController* PlayerController, int32 GroupIndex) const;

    FMAFeedback54 SendCommandToSelection(AMAPlayerController* PlayerController, EMACommand Command) const;
    FString GetCommandDisplayName(EMACommand Command) const;
    FMAFeedback54 TogglePauseExecution(AMAPlayerController* PlayerController) const;

    bool SpawnAgentByType(
        AMAPlayerController* PlayerController,
        const FString& AgentType,
        const FVector& Location,
        const FRotator& Rotation = FRotator::ZeroRotator) const;
    FMAAgentRuntimeStats GetAgentStats(const AMAPlayerController* PlayerController) const;
    bool DestroyLastAgent(AMAPlayerController* PlayerController, FString& OutAgentName) const;
    int32 JumpSelectedAgents(AMAPlayerController* PlayerController) const;
    void NavigateSelectedAgentsToLocation(AMAPlayerController* PlayerController, const FVector& TargetLocation) const;

    void SwitchToNextCamera(AMAPlayerController* PlayerController) const;
    void ReturnToSpectator(AMAPlayerController* PlayerController) const;
    bool TakePhotoForCurrentCamera(AMAPlayerController* PlayerController, FString& OutSensorName) const;
    FMACameraStreamRuntimeResult ToggleTCPStreamForCurrentCamera(AMAPlayerController* PlayerController) const;

    FMASquadOperationFeedback CycleFormation(AMAPlayerController* PlayerController) const;
    FMASquadOperationFeedback CreateSquadFromSelection(AMAPlayerController* PlayerController) const;
    FMASquadOperationFeedback DisbandSelectedSquads(AMAPlayerController* PlayerController) const;

    bool CanEnterEditMode(const AMAPlayerController* PlayerController) const;
    void ToggleEditSelection(AMAPlayerController* PlayerController, AActor* HitActor) const;
    void ClearEditSelection(AMAPlayerController* PlayerController) const;
    AMAPointOfInterest* CreatePOI(AMAPlayerController* PlayerController, const FVector& Location) const;
    void DestroyAllPOIs(AMAPlayerController* PlayerController) const;

private:
    class UMASelectionManager* ResolveSelectionManager(const AMAPlayerController* PlayerController) const;
    class UMAAgentManager* ResolveAgentManager(const AMAPlayerController* PlayerController) const;
    class UMACommandManager* ResolveCommandManager(const AMAPlayerController* PlayerController) const;
    class UMAViewportManager* ResolveViewportManager(const AMAPlayerController* PlayerController) const;
    class UMAEditModeManager* ResolveEditModeManager(const AMAPlayerController* PlayerController) const;
    class UMACameraSensorComponent* ResolveCurrentCamera(const AMAPlayerController* PlayerController) const;
    FVector ProjectToGround(AMAPlayerController* PlayerController, const FVector& WorldLocation) const;
};
