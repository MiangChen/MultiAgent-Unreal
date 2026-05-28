// Utility input handlers for debug, spawn stubs, and selection actions.

#include "MAAgentUtilityInputCoordinator.h"

#include "../../../Input/MAPlayerController.h"

void FMAAgentUtilityInputCoordinator::HandlePickup() const
{
    // Place skill owns pickup/drop behavior.
}

void FMAAgentUtilityInputCoordinator::HandleDrop() const
{
    // Place skill owns pickup/drop behavior.
}

FMAFeedback21Batch FMAAgentUtilityInputCoordinator::HandleSpawnPickupItem() const
{
    FMAFeedback21Batch Feedback;
    Feedback.AddMessage(TEXT("SpawnItem disabled - use Place skill"), EMAFeedback21MessageSeverity::Warning);
    return Feedback;
}

FMAFeedback21Batch FMAAgentUtilityInputCoordinator::HandleSpawnQuadruped() const
{
    FMAFeedback21Batch Feedback;
    Feedback.AddMessage(TEXT("SpawnQuadruped disabled - use config/agents.json"), EMAFeedback21MessageSeverity::Warning);
    return Feedback;
}

FMAFeedback21Batch FMAAgentUtilityInputCoordinator::HandlePrintAgentInfo(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    const FMAAgentRuntimeStats Stats = PlayerController->RuntimeGetAgentStats();

    Feedback.AddMessage(
        FString::Printf(TEXT("=== Agents: %d | UAVs: %d | Quadrupeds: %d | Humanoids: %d ==="),
            Stats.Total, Stats.UAVs, Stats.Quadrupeds, Stats.Humanoids),
        EMAFeedback21MessageSeverity::Info,
        5.f);

    for (const FString& Line : Stats.AgentLines)
    {
        Feedback.AddMessage(Line, EMAFeedback21MessageSeverity::Warning, 5.f);
    }

    return Feedback;
}

FMAFeedback21Batch FMAAgentUtilityInputCoordinator::HandleDestroyLastAgent(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    FString AgentName;
    if (PlayerController->RuntimeDestroyLastAgent(AgentName))
    {
        Feedback.AddMessage(
            FString::Printf(TEXT("Destroyed: %s"), *AgentName),
            EMAFeedback21MessageSeverity::Error,
            3.f);
    }

    return Feedback;
}

void FMAAgentUtilityInputCoordinator::HandleJumpSelection(AMAPlayerController* PlayerController) const
{
    const int32 JumpCount = PlayerController->RuntimeJumpSelectedAgents();
    if (JumpCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[Input] Jump: %d agents jumped"), JumpCount);
    }
}
