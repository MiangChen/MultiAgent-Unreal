// Deployment queue and placement coordination.

#include "MADeploymentInputCoordinator.h"

#include "../../../Input/MAPlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMADeploymentInputCoordinator, Log, All);

void FMADeploymentInputCoordinator::AddToQueue(
    AMAPlayerController* PlayerController,
    const FString& AgentType,
    int32 Count) const
{
    if (!PlayerController || Count <= 0)
    {
        return;
    }

    for (FMAPendingDeployment& Deployment : PlayerController->DeploymentState.Items)
    {
        if (Deployment.AgentType == AgentType)
        {
            Deployment.Count += Count;
            PlayerController->OnDeploymentQueueChanged.Broadcast();
            UE_LOG(LogMADeploymentInputCoordinator, Log, TEXT("Added %d x %s to queue (total: %d)"),
                Count, *AgentType, Deployment.Count);
            return;
        }
    }

    PlayerController->DeploymentState.Items.Add(FMAPendingDeployment(AgentType, Count));
    PlayerController->OnDeploymentQueueChanged.Broadcast();
    UE_LOG(LogMADeploymentInputCoordinator, Log, TEXT("Added new queue entry %d x %s"), Count, *AgentType);
}

void FMADeploymentInputCoordinator::RemoveFromQueue(
    AMAPlayerController* PlayerController,
    const FString& AgentType,
    int32 Count) const
{
    if (!PlayerController || Count <= 0)
    {
        return;
    }

    for (int32 Index = PlayerController->DeploymentState.Items.Num() - 1; Index >= 0; --Index)
    {
        FMAPendingDeployment& Deployment = PlayerController->DeploymentState.Items[Index];
        if (Deployment.AgentType != AgentType)
        {
            continue;
        }

        Deployment.Count -= Count;
        if (Deployment.Count <= 0)
        {
            PlayerController->DeploymentState.Items.RemoveAt(Index);
        }

        PlayerController->OnDeploymentQueueChanged.Broadcast();
        return;
    }
}

void FMADeploymentInputCoordinator::ClearQueue(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    PlayerController->DeploymentState.Items.Empty();
    PlayerController->DeploymentState.CurrentIndex = 0;
    PlayerController->DeploymentState.DeployedCount = 0;
    PlayerController->OnDeploymentQueueChanged.Broadcast();
    UE_LOG(LogMADeploymentInputCoordinator, Log, TEXT("Deployment queue cleared"));
}

int32 FMADeploymentInputCoordinator::GetQueueCount(const AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return 0;
    }

    int32 Total = 0;
    for (const FMAPendingDeployment& Deployment : PlayerController->DeploymentState.Items)
    {
        Total += Deployment.Count;
    }
    return Total;
}

FString FMADeploymentInputCoordinator::GetCurrentDeployingType(const AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->DeploymentState.Items.IsValidIndex(PlayerController->DeploymentState.CurrentIndex))
    {
        return TEXT("");
    }

    return PlayerController->DeploymentState.Items[PlayerController->DeploymentState.CurrentIndex].AgentType;
}

int32 FMADeploymentInputCoordinator::GetCurrentDeployingCount(const AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->DeploymentState.Items.IsValidIndex(PlayerController->DeploymentState.CurrentIndex))
    {
        return 0;
    }

    return PlayerController->DeploymentState.Items[PlayerController->DeploymentState.CurrentIndex].Count;
}

FMAFeedback21Batch FMADeploymentInputCoordinator::EnterMode(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    if (!HasPendingDeployments(PlayerController))
    {
        UE_LOG(LogMADeploymentInputCoordinator, Warning, TEXT("EnterMode: no pending deployments"));
        Feedback.AddMessage(TEXT("No units to deploy!"), EMAFeedback21MessageSeverity::Warning, 3.f);
        return Feedback;
    }

    PlayerController->DeploymentState.CurrentIndex = 0;
    PlayerController->DeploymentState.DeployedCount = 0;

    Feedback.Append(PlayerController->MouseModeCoordinator.TransitionToMode(PlayerController, EMAMouseMode::Deployment));

    const int32 TotalCount = GetQueueCount(PlayerController);
    UE_LOG(LogMADeploymentInputCoordinator, Log, TEXT("Entered deployment mode: %d types, %d total"),
        PlayerController->DeploymentState.Items.Num(), TotalCount);

    Feedback.AddMessage(
        FString::Printf(TEXT("Deployment Mode: Drag to place %d agents"), TotalCount),
        EMAFeedback21MessageSeverity::Info,
        5.f);
    return Feedback;
}

void FMADeploymentInputCoordinator::EnterModeWithUnits(
    AMAPlayerController* PlayerController,
    const TArray<FMAPendingDeployment>& Deployments) const
{
    if (!PlayerController)
    {
        return;
    }

    for (const FMAPendingDeployment& Deployment : Deployments)
    {
        AddToQueue(PlayerController, Deployment.AgentType, Deployment.Count);
    }

    EnterMode(PlayerController);
}

FMAFeedback21Batch FMADeploymentInputCoordinator::ExitMode(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController)
    {
        return Feedback;
    }

    if (RuntimeAdapter.IsSelectionBoxActive(PlayerController))
    {
        RuntimeAdapter.CancelSelectionBox(PlayerController);
    }

    const EMAMouseMode ResumeMode = PlayerController->MouseModeState.PreviousMode == EMAMouseMode::Deployment
        ? EMAMouseMode::Select
        : PlayerController->MouseModeState.PreviousMode;
    Feedback.Append(PlayerController->MouseModeCoordinator.TransitionToMode(PlayerController, ResumeMode));

    const int32 RemainingCount = GetQueueCount(PlayerController);

    UE_LOG(LogMADeploymentInputCoordinator, Log, TEXT("Exited deployment mode, deployed=%d remaining=%d"),
        PlayerController->DeploymentState.DeployedCount, RemainingCount);

    if (RemainingCount > 0)
    {
        Feedback.AddMessage(
            FString::Printf(TEXT("Deployment paused. %d units remaining in queue."), RemainingCount),
            EMAFeedback21MessageSeverity::Warning,
            3.f);
    }
    else
    {
        Feedback.AddMessage(TEXT("All units deployed!"), EMAFeedback21MessageSeverity::Success, 3.f);
        PlayerController->OnDeploymentCompleted.Broadcast();
    }

    ResetTransientState(PlayerController);
    return Feedback;
}

void FMADeploymentInputCoordinator::HandleLeftClickStarted(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        RuntimeAdapter.BeginSelectionBox(PlayerController, FVector2D(MouseX, MouseY));
    }
}

FMAFeedback21Batch FMADeploymentInputCoordinator::HandleLeftClickReleased(AMAPlayerController* PlayerController) const
{
    FMAFeedback21Batch Feedback;
    if (!PlayerController
        || !RuntimeAdapter.IsSelectionBoxActive(PlayerController))
    {
        return Feedback;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        RuntimeAdapter.UpdateSelectionBox(PlayerController, FVector2D(MouseX, MouseY));
    }

    const FVector2D Start = RuntimeAdapter.GetSelectionBoxStart(PlayerController);
    const FVector2D End = RuntimeAdapter.GetSelectionBoxEnd(PlayerController);
    RuntimeAdapter.CancelSelectionBox(PlayerController);

    const float BoxWidth = FMath::Abs(End.X - Start.X);
    const float BoxHeight = FMath::Abs(End.Y - Start.Y);
    if (BoxWidth < 20.f && BoxHeight < 20.f)
    {
        Feedback.AddMessage(TEXT("Drag a larger area to deploy"), EMAFeedback21MessageSeverity::Warning);
        return Feedback;
    }

    if (!PlayerController->DeploymentState.Items.IsValidIndex(PlayerController->DeploymentState.CurrentIndex))
    {
        Feedback.Append(ExitMode(PlayerController));
        return Feedback;
    }

    FMAPendingDeployment& CurrentDeployment = PlayerController->DeploymentState.Items[PlayerController->DeploymentState.CurrentIndex];
    const TArray<FVector> SpawnPoints = RuntimeAdapter.ProjectSelectionBoxToWorld(
        PlayerController,
        Start,
        End,
        CurrentDeployment.Count);

    int32 SpawnedThisBatch = 0;
    for (const FVector& SpawnPoint : SpawnPoints)
    {
        if (RuntimeAdapter.SpawnAgentByType(PlayerController, CurrentDeployment.AgentType, SpawnPoint))
        {
            SpawnedThisBatch++;
            PlayerController->DeploymentState.DeployedCount++;
        }
    }

    UE_LOG(LogMADeploymentInputCoordinator, Log, TEXT("Deployed %d x %s"),
        SpawnedThisBatch, *CurrentDeployment.AgentType);
    Feedback.AddMessage(
        FString::Printf(TEXT("Deployed %d x %s"), SpawnedThisBatch, *CurrentDeployment.AgentType),
        EMAFeedback21MessageSeverity::Success,
        3.f);

    PlayerController->DeploymentState.Items.RemoveAt(PlayerController->DeploymentState.CurrentIndex);
    PlayerController->OnDeploymentQueueChanged.Broadcast();

    if (PlayerController->DeploymentState.Items.IsEmpty())
    {
        Feedback.Append(ExitMode(PlayerController));
        return Feedback;
    }

    if (!PlayerController->DeploymentState.Items.IsValidIndex(PlayerController->DeploymentState.CurrentIndex))
    {
        PlayerController->DeploymentState.CurrentIndex = 0;
    }

    const FMAPendingDeployment& NextDeployment = PlayerController->DeploymentState.Items[PlayerController->DeploymentState.CurrentIndex];
    Feedback.AddMessage(
        FString::Printf(TEXT("Next: Drag to place %d x %s"), NextDeployment.Count, *NextDeployment.AgentType),
        EMAFeedback21MessageSeverity::Info,
        5.f);
    return Feedback;
}

void FMADeploymentInputCoordinator::ResetTransientState(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    PlayerController->DeploymentState.CurrentIndex = 0;
    PlayerController->DeploymentState.DeployedCount = 0;
}

bool FMADeploymentInputCoordinator::HasPendingDeployments(const AMAPlayerController* PlayerController) const
{
    return PlayerController
        && !PlayerController->DeploymentState.Items.IsEmpty()
        && GetQueueCount(PlayerController) > 0;
}
