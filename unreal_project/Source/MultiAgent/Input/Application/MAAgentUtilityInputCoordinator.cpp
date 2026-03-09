// Utility input handlers for debug, spawn stubs, and selection actions.

#include "MAAgentUtilityInputCoordinator.h"

#include "../MAPlayerController.h"
#include "../../Core/Manager/MAAgentManager.h"
#include "../../Core/Manager/MASelectionManager.h"
#include "../../Agent/Character/MACharacter.h"

void FMAAgentUtilityInputCoordinator::HandlePickup() const
{
    // Place skill owns pickup/drop behavior.
}

void FMAAgentUtilityInputCoordinator::HandleDrop() const
{
    // Place skill owns pickup/drop behavior.
}

void FMAAgentUtilityInputCoordinator::HandleSpawnPickupItem() const
{
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("SpawnItem disabled - use Place skill"));
}

void FMAAgentUtilityInputCoordinator::HandleSpawnQuadruped() const
{
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("SpawnQuadruped disabled - use config/agents.json"));
}

void FMAAgentUtilityInputCoordinator::HandlePrintAgentInfo(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->AgentManager)
    {
        return;
    }

    const int32 Total = PlayerController->AgentManager->GetAgentCount();
    const int32 Quadrupeds = PlayerController->AgentManager->GetAgentsByType(EMAAgentType::Quadruped).Num();
    const int32 Humanoids = PlayerController->AgentManager->GetAgentsByType(EMAAgentType::Humanoid).Num();
    const int32 UAVs = PlayerController->AgentManager->GetAgentsByType(EMAAgentType::UAV).Num();

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
        FString::Printf(TEXT("=== Agents: %d | UAVs: %d | Quadrupeds: %d | Humanoids: %d ==="),
            Total, UAVs, Quadrupeds, Humanoids));

    for (AMACharacter* Agent : PlayerController->AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            const FString Info = FString::Printf(TEXT("  [%s] %s"), *Agent->AgentID, *Agent->AgentLabel);
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Info);
        }
    }
}

void FMAAgentUtilityInputCoordinator::HandleDestroyLastAgent(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->AgentManager)
    {
        return;
    }

    const TArray<AMACharacter*> AllAgents = PlayerController->AgentManager->GetAllAgents();
    if (AllAgents.IsEmpty())
    {
        return;
    }

    AMACharacter* LastAgent = AllAgents.Last();
    const FString Name = LastAgent ? LastAgent->AgentLabel : TEXT("Unknown");
    if (LastAgent && PlayerController->AgentManager->DestroyAgent(LastAgent))
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            FString::Printf(TEXT("Destroyed: %s"), *Name));
    }
}

void FMAAgentUtilityInputCoordinator::HandleJumpSelection(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->SelectionManager)
    {
        return;
    }

    const TArray<AMACharacter*> SelectedAgents = PlayerController->SelectionManager->GetSelectedAgents();
    if (SelectedAgents.IsEmpty())
    {
        return;
    }

    int32 JumpCount = 0;
    for (AMACharacter* Agent : SelectedAgents)
    {
        if (Agent && Agent->CanJump())
        {
            Agent->Jump();
            JumpCount++;
        }
    }

    if (JumpCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[Input] Jump: %d agents jumped"), JumpCount);
    }
}
