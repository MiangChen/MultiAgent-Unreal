// Deployment queue and placement coordination.

#include "MADeploymentInputCoordinator.h"

#include "../MAPlayerController.h"
#include "../../Core/Manager/MAAgentManager.h"
#include "../../Core/Manager/MASelectionManager.h"

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

    for (FMAPendingDeployment& Deployment : PlayerController->DeploymentQueue)
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

    PlayerController->DeploymentQueue.Add(FMAPendingDeployment(AgentType, Count));
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

    for (int32 Index = PlayerController->DeploymentQueue.Num() - 1; Index >= 0; --Index)
    {
        FMAPendingDeployment& Deployment = PlayerController->DeploymentQueue[Index];
        if (Deployment.AgentType != AgentType)
        {
            continue;
        }

        Deployment.Count -= Count;
        if (Deployment.Count <= 0)
        {
            PlayerController->DeploymentQueue.RemoveAt(Index);
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

    PlayerController->DeploymentQueue.Empty();
    PlayerController->CurrentDeploymentIndex = 0;
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
    for (const FMAPendingDeployment& Deployment : PlayerController->DeploymentQueue)
    {
        Total += Deployment.Count;
    }
    return Total;
}

FString FMADeploymentInputCoordinator::GetCurrentDeployingType(const AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->DeploymentQueue.IsValidIndex(PlayerController->CurrentDeploymentIndex))
    {
        return TEXT("");
    }

    return PlayerController->DeploymentQueue[PlayerController->CurrentDeploymentIndex].AgentType;
}

int32 FMADeploymentInputCoordinator::GetCurrentDeployingCount(const AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->DeploymentQueue.IsValidIndex(PlayerController->CurrentDeploymentIndex))
    {
        return 0;
    }

    return PlayerController->DeploymentQueue[PlayerController->CurrentDeploymentIndex].Count;
}

void FMADeploymentInputCoordinator::EnterMode(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    if (!HasPendingDeployments(PlayerController))
    {
        UE_LOG(LogMADeploymentInputCoordinator, Warning, TEXT("EnterMode: no pending deployments"));
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("No units to deploy!"));
        return;
    }

    PlayerController->CurrentDeploymentIndex = 0;
    PlayerController->DeployedCount = 0;

    if (!PlayerController->MouseModeCoordinator.TransitionToMode(PlayerController, EMAMouseMode::Deployment))
    {
        return;
    }

    const int32 TotalCount = GetQueueCount(PlayerController);
    UE_LOG(LogMADeploymentInputCoordinator, Log, TEXT("Entered deployment mode: %d types, %d total"),
        PlayerController->DeploymentQueue.Num(), TotalCount);

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
        FString::Printf(TEXT("Deployment Mode: Drag to place %d agents"), TotalCount));
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

void FMADeploymentInputCoordinator::ExitMode(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    if (PlayerController->SelectionManager && PlayerController->SelectionManager->IsBoxSelecting())
    {
        PlayerController->SelectionManager->CancelBoxSelect();
    }

    const EMAMouseMode ResumeMode = PlayerController->PreviousMouseMode == EMAMouseMode::Deployment
        ? EMAMouseMode::Select
        : PlayerController->PreviousMouseMode;
    PlayerController->MouseModeCoordinator.TransitionToMode(PlayerController, ResumeMode);

    const int32 RemainingCount = GetQueueCount(PlayerController);

    UE_LOG(LogMADeploymentInputCoordinator, Log, TEXT("Exited deployment mode, deployed=%d remaining=%d"),
        PlayerController->DeployedCount, RemainingCount);

    if (RemainingCount > 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
            FString::Printf(TEXT("Deployment paused. %d units remaining in queue."), RemainingCount));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("All units deployed!"));
        PlayerController->OnDeploymentCompleted.Broadcast();
    }

    ResetTransientState(PlayerController);
}

void FMADeploymentInputCoordinator::HandleLeftClickStarted(AMAPlayerController* PlayerController) const
{
    if (!PlayerController || !PlayerController->SelectionManager)
    {
        return;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        PlayerController->SelectionManager->BeginBoxSelect(FVector2D(MouseX, MouseY));
    }
}

void FMADeploymentInputCoordinator::HandleLeftClickReleased(AMAPlayerController* PlayerController) const
{
    if (!PlayerController
        || !PlayerController->SelectionManager
        || !PlayerController->SelectionManager->IsBoxSelecting()
        || !PlayerController->AgentManager)
    {
        return;
    }

    float MouseX = 0.f;
    float MouseY = 0.f;
    if (PlayerController->GetMousePosition(MouseX, MouseY))
    {
        PlayerController->SelectionManager->UpdateBoxSelect(FVector2D(MouseX, MouseY));
    }

    const FVector2D Start = PlayerController->SelectionManager->GetBoxSelectStart();
    const FVector2D End = PlayerController->SelectionManager->GetBoxSelectEnd();
    PlayerController->SelectionManager->CancelBoxSelect();

    const float BoxWidth = FMath::Abs(End.X - Start.X);
    const float BoxHeight = FMath::Abs(End.Y - Start.Y);
    if (BoxWidth < 20.f && BoxHeight < 20.f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Drag a larger area to deploy"));
        return;
    }

    if (!PlayerController->DeploymentQueue.IsValidIndex(PlayerController->CurrentDeploymentIndex))
    {
        ExitMode(PlayerController);
        return;
    }

    FMAPendingDeployment& CurrentDeployment = PlayerController->DeploymentQueue[PlayerController->CurrentDeploymentIndex];
    const TArray<FVector> SpawnPoints = ProjectSelectionBoxToWorld(
        PlayerController,
        Start,
        End,
        CurrentDeployment.Count);

    int32 SpawnedThisBatch = 0;
    for (const FVector& SpawnPoint : SpawnPoints)
    {
        if (PlayerController->AgentManager->SpawnAgentByType(
            CurrentDeployment.AgentType,
            SpawnPoint,
            FRotator::ZeroRotator,
            false))
        {
            SpawnedThisBatch++;
            PlayerController->DeployedCount++;
        }
    }

    UE_LOG(LogMADeploymentInputCoordinator, Log, TEXT("Deployed %d x %s"),
        SpawnedThisBatch, *CurrentDeployment.AgentType);
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        FString::Printf(TEXT("Deployed %d x %s"), SpawnedThisBatch, *CurrentDeployment.AgentType));

    PlayerController->DeploymentQueue.RemoveAt(PlayerController->CurrentDeploymentIndex);
    PlayerController->OnDeploymentQueueChanged.Broadcast();

    if (PlayerController->DeploymentQueue.IsEmpty())
    {
        ExitMode(PlayerController);
        return;
    }

    if (!PlayerController->DeploymentQueue.IsValidIndex(PlayerController->CurrentDeploymentIndex))
    {
        PlayerController->CurrentDeploymentIndex = 0;
    }

    const FMAPendingDeployment& NextDeployment = PlayerController->DeploymentQueue[PlayerController->CurrentDeploymentIndex];
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
        FString::Printf(TEXT("Next: Drag to place %d x %s"), NextDeployment.Count, *NextDeployment.AgentType));
}

void FMADeploymentInputCoordinator::ResetTransientState(AMAPlayerController* PlayerController) const
{
    if (!PlayerController)
    {
        return;
    }

    PlayerController->CurrentDeploymentIndex = 0;
    PlayerController->DeployedCount = 0;
}

bool FMADeploymentInputCoordinator::HasPendingDeployments(const AMAPlayerController* PlayerController) const
{
    return PlayerController
        && !PlayerController->DeploymentQueue.IsEmpty()
        && GetQueueCount(PlayerController) > 0;
}

TArray<FVector> FMADeploymentInputCoordinator::ProjectSelectionBoxToWorld(
    AMAPlayerController* PlayerController,
    const FVector2D& Start,
    const FVector2D& End,
    int32 Count) const
{
    TArray<FVector> Points;
    if (!PlayerController || Count <= 0)
    {
        return Points;
    }

    const float MinX = FMath::Min(Start.X, End.X);
    const float MaxX = FMath::Max(Start.X, End.X);
    const float MinY = FMath::Min(Start.Y, End.Y);
    const float MaxY = FMath::Max(Start.Y, End.Y);

    const float BoxWidth = MaxX - MinX;
    const float BoxHeight = MaxY - MinY;

    UE_LOG(LogMADeploymentInputCoordinator, Verbose, TEXT("Screen box: (%.0f, %.0f) -> (%.0f, %.0f), %.0f x %.0f"),
        MinX, MinY, MaxX, MaxY, BoxWidth, BoxHeight);

    const int32 Cols = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count))));
    const int32 Rows = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Count) / Cols));

    int32 Spawned = 0;
    for (int32 Row = 0; Row < Rows && Spawned < Count; ++Row)
    {
        for (int32 Col = 0; Col < Cols && Spawned < Count; ++Col)
        {
            float U = Cols > 1 ? (static_cast<float>(Col) / (Cols - 1)) : 0.5f;
            float V = Rows > 1 ? (static_cast<float>(Row) / (Rows - 1)) : 0.5f;

            U = FMath::Clamp(U + FMath::RandRange(-0.05f, 0.05f), 0.f, 1.f);
            V = FMath::Clamp(V + FMath::RandRange(-0.05f, 0.05f), 0.f, 1.f);

            const float ScreenX = FMath::Lerp(MinX, MaxX, U);
            const float ScreenY = FMath::Lerp(MinY, MaxY, V);

            FVector WorldPos;
            FVector WorldDir;
            PlayerController->DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldPos, WorldDir);

            FHitResult HitResult;
            const FVector TraceStart = WorldPos;
            const FVector TraceEnd = WorldPos + WorldDir * 50000.f;

            FVector SpawnPoint;
            if (PlayerController->GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
            {
                SpawnPoint = HitResult.Location;
            }
            else
            {
                SpawnPoint = ProjectToGround(PlayerController, WorldPos + WorldDir * 5000.f);
            }

            Points.Add(SpawnPoint);
            Spawned++;
        }
    }

    return Points;
}

FVector FMADeploymentInputCoordinator::ProjectToGround(
    AMAPlayerController* PlayerController,
    const FVector& WorldLocation) const
{
    UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
    if (!World)
    {
        return WorldLocation;
    }

    FHitResult HitResult;
    const FVector TraceStart(WorldLocation.X, WorldLocation.Y, 10000.f);
    const FVector TraceEnd(WorldLocation.X, WorldLocation.Y, -20000.f);

    if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
    {
        return HitResult.Location;
    }

    UE_LOG(LogMADeploymentInputCoordinator, Warning, TEXT("ProjectToGround: no ground at (%.0f, %.0f)"),
        WorldLocation.X, WorldLocation.Y);
    return WorldLocation;
}
