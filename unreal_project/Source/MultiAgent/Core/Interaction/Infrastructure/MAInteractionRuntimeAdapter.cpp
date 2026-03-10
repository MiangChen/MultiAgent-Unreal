// L4 bridge for input-side runtime access to UE managers, world queries, and actors.

#include "MAInteractionRuntimeAdapter.h"

#include "../../../Input/MAPlayerController.h"
#include "../../AgentRuntime/Runtime/MAAgentManager.h"
#include "../../Command/Runtime/MACommandManager.h"
#include "../../Editing/Runtime/MAEditModeManager.h"
#include "../../Selection/Runtime/MASelectionManager.h"
#include "../../Squad/Runtime/MASquadManager.h"
#include "../../Camera/Runtime/MAViewportManager.h"
#include "../../Squad/Domain/MASquad.h"
#include "../../../Agent/Character/MACharacter.h"
#include "../../../Agent/Character/MAUAVCharacter.h"
#include "../../../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "../../../Environment/Utils/MAPointOfInterest.h"

namespace
{
template <typename TSubsystem>
TSubsystem* ResolveWorldSubsystem(const AMAPlayerController* PlayerController)
{
    if (const UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr)
    {
        return World->GetSubsystem<TSubsystem>();
    }

    return nullptr;
}
}

UMASelectionManager* FMAInteractionRuntimeAdapter::ResolveSelectionManager(const AMAPlayerController* PlayerController) const
{
    return ResolveWorldSubsystem<UMASelectionManager>(PlayerController);
}

UMAAgentManager* FMAInteractionRuntimeAdapter::ResolveAgentManager(const AMAPlayerController* PlayerController) const
{
    return ResolveWorldSubsystem<UMAAgentManager>(PlayerController);
}

UMACommandManager* FMAInteractionRuntimeAdapter::ResolveCommandManager(const AMAPlayerController* PlayerController) const
{
    return ResolveWorldSubsystem<UMACommandManager>(PlayerController);
}

UMASquadManager* FMAInteractionRuntimeAdapter::ResolveSquadManager(const AMAPlayerController* PlayerController) const
{
    return ResolveWorldSubsystem<UMASquadManager>(PlayerController);
}

UMAViewportManager* FMAInteractionRuntimeAdapter::ResolveViewportManager(const AMAPlayerController* PlayerController) const
{
    return ResolveWorldSubsystem<UMAViewportManager>(PlayerController);
}

UMAEditModeManager* FMAInteractionRuntimeAdapter::ResolveEditModeManager(const AMAPlayerController* PlayerController) const
{
    return ResolveWorldSubsystem<UMAEditModeManager>(PlayerController);
}

UMACameraSensorComponent* FMAInteractionRuntimeAdapter::ResolveCurrentCamera(const AMAPlayerController* PlayerController) const
{
    if (UMAViewportManager* ViewportManager = ResolveViewportManager(PlayerController))
    {
        return ViewportManager->GetCurrentCamera();
    }

    return nullptr;
}

bool FMAInteractionRuntimeAdapter::IsSelectionBoxActive(const AMAPlayerController* PlayerController) const
{
    if (const UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        return SelectionManager->IsBoxSelecting();
    }

    return false;
}

void FMAInteractionRuntimeAdapter::BeginSelectionBox(AMAPlayerController* PlayerController, const FVector2D& Start) const
{
    if (UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        SelectionManager->BeginBoxSelect(Start);
    }
}

void FMAInteractionRuntimeAdapter::UpdateSelectionBox(AMAPlayerController* PlayerController, const FVector2D& Current) const
{
    if (UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        SelectionManager->UpdateBoxSelect(Current);
    }
}

void FMAInteractionRuntimeAdapter::EndSelectionBox(AMAPlayerController* PlayerController, const bool bAppendSelection) const
{
    if (UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        SelectionManager->EndBoxSelect(PlayerController, bAppendSelection);
    }
}

void FMAInteractionRuntimeAdapter::CancelSelectionBox(AMAPlayerController* PlayerController) const
{
    if (UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        SelectionManager->CancelBoxSelect();
    }
}

FVector2D FMAInteractionRuntimeAdapter::GetSelectionBoxStart(const AMAPlayerController* PlayerController) const
{
    if (const UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        return SelectionManager->GetBoxSelectStart();
    }

    return FVector2D::ZeroVector;
}

FVector2D FMAInteractionRuntimeAdapter::GetSelectionBoxEnd(const AMAPlayerController* PlayerController) const
{
    if (const UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        return SelectionManager->GetBoxSelectEnd();
    }

    return FVector2D::ZeroVector;
}

bool FMAInteractionRuntimeAdapter::ResolveCursorHit(
    AMAPlayerController* PlayerController,
    const ECollisionChannel CollisionChannel,
    FHitResult& OutHitResult) const
{
    return PlayerController
        && PlayerController->GetHitResultUnderCursor(CollisionChannel, false, OutHitResult);
}

bool FMAInteractionRuntimeAdapter::ResolveMouseHitLocation(AMAPlayerController* PlayerController, FVector& OutLocation) const
{
    if (!PlayerController)
    {
        return false;
    }

    FHitResult HitResult;
    if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
    {
        OutLocation = HitResult.Location;
        return true;
    }

    return false;
}

TArray<FVector> FMAInteractionRuntimeAdapter::ProjectSelectionBoxToWorld(
    AMAPlayerController* PlayerController,
    const FVector2D& Start,
    const FVector2D& End,
    const int32 Count) const
{
    TArray<FVector> Points;
    if (!PlayerController || Count <= 0)
    {
        return Points;
    }

    FVector WorldOrigin1;
    FVector WorldDirection1;
    FVector WorldOrigin2;
    FVector WorldDirection2;

    PlayerController->DeprojectScreenPositionToWorld(Start.X, Start.Y, WorldOrigin1, WorldDirection1);
    PlayerController->DeprojectScreenPositionToWorld(End.X, End.Y, WorldOrigin2, WorldDirection2);

    const FVector Corner1 = WorldOrigin1 + WorldDirection1 * 1000.f;
    const FVector Corner2 = WorldOrigin2 + WorldDirection2 * 1000.f;

    const FVector MinCorner = FVector(
        FMath::Min(Corner1.X, Corner2.X),
        FMath::Min(Corner1.Y, Corner2.Y),
        0.f);
    const FVector MaxCorner = FVector(
        FMath::Max(Corner1.X, Corner2.X),
        FMath::Max(Corner1.Y, Corner2.Y),
        0.f);

    const int32 Rows = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
    const int32 Cols = FMath::CeilToInt(static_cast<float>(Count) / Rows);

    for (int32 Row = 0; Row < Rows && Points.Num() < Count; ++Row)
    {
        for (int32 Col = 0; Col < Cols && Points.Num() < Count; ++Col)
        {
            const float AlphaX = Cols > 1 ? static_cast<float>(Col) / (Cols - 1) : 0.5f;
            const float AlphaY = Rows > 1 ? static_cast<float>(Row) / (Rows - 1) : 0.5f;

            FVector Candidate = FMath::Lerp(MinCorner, MaxCorner, FVector(AlphaX, AlphaY, 0.f));
            Candidate.Z = 1000.f;
            Points.Add(ProjectToGround(PlayerController, Candidate));
        }
    }

    return Points;
}

AMACharacter* FMAInteractionRuntimeAdapter::ResolveClickedAgent(AMAPlayerController* PlayerController) const
{
    FHitResult HitResult;
    if (ResolveCursorHit(PlayerController, ECC_Pawn, HitResult))
    {
        return Cast<AMACharacter>(HitResult.GetActor());
    }

    return nullptr;
}

void FMAInteractionRuntimeAdapter::ToggleSelection(AMAPlayerController* PlayerController, AMACharacter* Agent) const
{
    if (Agent)
    {
        if (UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
        {
            SelectionManager->ToggleSelection(Agent);
        }
    }
}

void FMAInteractionRuntimeAdapter::SelectAgent(AMAPlayerController* PlayerController, AMACharacter* Agent) const
{
    if (Agent)
    {
        if (UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
        {
            SelectionManager->SelectAgent(Agent);
        }
    }
}

TArray<AMACharacter*> FMAInteractionRuntimeAdapter::GetSelectedAgents(const AMAPlayerController* PlayerController) const
{
    if (const UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        return SelectionManager->GetSelectedAgents();
    }

    return {};
}

void FMAInteractionRuntimeAdapter::SaveToControlGroup(AMAPlayerController* PlayerController, const int32 GroupIndex) const
{
    if (UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        SelectionManager->SaveToControlGroup(GroupIndex);
    }
}

void FMAInteractionRuntimeAdapter::SelectControlGroup(AMAPlayerController* PlayerController, const int32 GroupIndex) const
{
    if (UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController))
    {
        SelectionManager->SelectControlGroup(GroupIndex);
    }
}

FMAFeedback54 FMAInteractionRuntimeAdapter::SendCommandToSelection(
    AMAPlayerController* PlayerController,
    const EMACommand Command) const
{
    FMAFeedback54 Feedback54;
    Feedback54.Observation = EMAFeedback54Observation::CommandDispatch;
    Feedback54.SubjectId = GetCommandDisplayName(Command);

    UMASelectionManager* SelectionManager = ResolveSelectionManager(PlayerController);
    UMACommandManager* CommandManager = ResolveCommandManager(PlayerController);
    if (!SelectionManager || !CommandManager || Command == EMACommand::None)
    {
        Feedback54.Message = TEXT("Command runtime unavailable");
        return Feedback54;
    }

    const TArray<AMACharacter*> SelectedAgents = SelectionManager->GetSelectedAgents();
    Feedback54.Count = SelectedAgents.Num();
    if (SelectedAgents.IsEmpty())
    {
        Feedback54.Message = TEXT("No agents selected");
        return Feedback54;
    }

    for (AMACharacter* Agent : SelectedAgents)
    {
        if (Agent)
        {
            CommandManager->SendCommand(Agent, Command);
        }
    }

    Feedback54.bSuccess = true;
    Feedback54.bHasObservation = true;
    Feedback54.Message = FString::Printf(TEXT("%s -> %d agent(s)"), *Feedback54.SubjectId, Feedback54.Count);
    return Feedback54;
}

FString FMAInteractionRuntimeAdapter::GetCommandDisplayName(const EMACommand Command) const
{
    return UMACommandManager::CommandToString(Command);
}

FMAFeedback54 FMAInteractionRuntimeAdapter::TogglePauseExecution(AMAPlayerController* PlayerController) const
{
    FMAFeedback54 Feedback54;
    Feedback54.Observation = EMAFeedback54Observation::PauseToggle;
    Feedback54.SubjectId = TEXT("PauseExecution");

    if (UMACommandManager* CommandManager = ResolveCommandManager(PlayerController))
    {
        CommandManager->TogglePauseExecution();
        Feedback54.bSuccess = true;
        Feedback54.bHasObservation = true;
        Feedback54.Message = TEXT("Toggled skill execution pause state");
    }

    return Feedback54;
}

bool FMAInteractionRuntimeAdapter::SpawnAgentByType(
    AMAPlayerController* PlayerController,
    const FString& AgentType,
    const FVector& Location,
    const FRotator& Rotation) const
{
    if (UMAAgentManager* AgentManager = ResolveAgentManager(PlayerController))
    {
        return AgentManager->SpawnAgentByType(AgentType, Location, Rotation, false);
    }

    return false;
}

FMAAgentRuntimeStats FMAInteractionRuntimeAdapter::GetAgentStats(const AMAPlayerController* PlayerController) const
{
    FMAAgentRuntimeStats Stats;
    if (const UMAAgentManager* AgentManager = ResolveAgentManager(PlayerController))
    {
        Stats.Total = AgentManager->GetAgentCount();
        Stats.Quadrupeds = AgentManager->GetAgentsByType(EMAAgentType::Quadruped).Num();
        Stats.Humanoids = AgentManager->GetAgentsByType(EMAAgentType::Humanoid).Num();
        Stats.UAVs = AgentManager->GetAgentsByType(EMAAgentType::UAV).Num();

        for (AMACharacter* Agent : AgentManager->GetAllAgents())
        {
            if (Agent)
            {
                Stats.AgentLines.Add(FString::Printf(TEXT("  [%s] %s"), *Agent->AgentID, *Agent->AgentLabel));
            }
        }
    }

    return Stats;
}

bool FMAInteractionRuntimeAdapter::DestroyLastAgent(AMAPlayerController* PlayerController, FString& OutAgentName) const
{
    if (UMAAgentManager* AgentManager = ResolveAgentManager(PlayerController))
    {
        const TArray<AMACharacter*> AllAgents = AgentManager->GetAllAgents();
        if (!AllAgents.IsEmpty())
        {
            AMACharacter* LastAgent = AllAgents.Last();
            OutAgentName = LastAgent ? LastAgent->AgentLabel : TEXT("Unknown");
            return LastAgent && AgentManager->DestroyAgent(LastAgent);
        }
    }

    return false;
}

int32 FMAInteractionRuntimeAdapter::JumpSelectedAgents(AMAPlayerController* PlayerController) const
{
    int32 JumpCount = 0;
    for (AMACharacter* Agent : GetSelectedAgents(PlayerController))
    {
        if (Agent && Agent->CanJump())
        {
            Agent->Jump();
            JumpCount++;
        }
    }

    return JumpCount;
}

void FMAInteractionRuntimeAdapter::NavigateSelectedAgentsToLocation(
    AMAPlayerController* PlayerController,
    const FVector& TargetLocation) const
{
    for (AMACharacter* Agent : GetSelectedAgents(PlayerController))
    {
        if (!Agent)
        {
            continue;
        }

        FVector Target = TargetLocation;
        if (Agent->AgentType == EMAAgentType::UAV)
        {
            if (const AMAUAVCharacter* UAV = Cast<AMAUAVCharacter>(Agent); UAV && UAV->IsInAir())
            {
                Target.Z = Agent->GetActorLocation().Z;
            }
        }

        Agent->TryNavigateTo(Target);
    }
}

void FMAInteractionRuntimeAdapter::SwitchToNextCamera(AMAPlayerController* PlayerController) const
{
    if (UMAViewportManager* ViewportManager = ResolveViewportManager(PlayerController))
    {
        ViewportManager->SwitchToNextCamera(PlayerController);
    }
}

void FMAInteractionRuntimeAdapter::ReturnToSpectator(AMAPlayerController* PlayerController) const
{
    if (UMAViewportManager* ViewportManager = ResolveViewportManager(PlayerController))
    {
        ViewportManager->ReturnToSpectator(PlayerController);
    }
}

bool FMAInteractionRuntimeAdapter::TakePhotoForCurrentCamera(
    AMAPlayerController* PlayerController,
    FString& OutSensorName) const
{
    if (UMACameraSensorComponent* Camera = ResolveCurrentCamera(PlayerController))
    {
        if (Camera->TakePhoto())
        {
            OutSensorName = Camera->SensorName;
            return true;
        }
    }

    return false;
}

FMACameraStreamRuntimeResult FMAInteractionRuntimeAdapter::ToggleTCPStreamForCurrentCamera(AMAPlayerController* PlayerController) const
{
    FMACameraStreamRuntimeResult Result;
    UMACameraSensorComponent* Camera = ResolveCurrentCamera(PlayerController);
    if (!Camera)
    {
        return Result;
    }

    if (const UMAAgentManager* AgentManager = ResolveAgentManager(PlayerController))
    {
        for (AMACharacter* Agent : AgentManager->GetAllAgents())
        {
            if (!Agent)
            {
                continue;
            }

            if (UMACameraSensorComponent* OtherCamera = Agent->GetCameraSensor())
            {
                if (OtherCamera != Camera && OtherCamera->bIsStreaming)
                {
                    OtherCamera->StopTCPStream();
                }
            }
        }
    }

    Result.SensorName = Camera->SensorName;
    if (Camera->bIsStreaming)
    {
        Camera->StopTCPStream();
        Result.Event = EMACameraStreamEvent::Stopped;
        return Result;
    }

    if (Camera->StartTCPStream(9000, Camera->StreamFPS))
    {
        Result.Event = EMACameraStreamEvent::Started;
    }

    return Result;
}

void FMAInteractionRuntimeAdapter::CycleFormation(AMAPlayerController* PlayerController) const
{
    if (UMASquadManager* SquadManager = ResolveSquadManager(PlayerController))
    {
        if (UMASquad* Squad = SquadManager->GetOrCreateDefaultSquad())
        {
            SquadManager->CycleFormation(Squad);
        }
    }
}

bool FMAInteractionRuntimeAdapter::CreateSquadFromSelection(
    AMAPlayerController* PlayerController,
    FString& OutSquadName,
    int32& OutMemberCount) const
{
    UMASquadManager* SquadManager = ResolveSquadManager(PlayerController);
    const TArray<AMACharacter*> Selected = GetSelectedAgents(PlayerController);
    if (!SquadManager || Selected.Num() < 2)
    {
        return false;
    }

    if (UMASquad* Squad = SquadManager->CreateSquad(Selected, Selected[0]))
    {
        OutSquadName = Squad->SquadName;
        OutMemberCount = Squad->GetMemberCount();
        return true;
    }

    return false;
}

int32 FMAInteractionRuntimeAdapter::DisbandSelectedSquads(AMAPlayerController* PlayerController) const
{
    UMASquadManager* SquadManager = ResolveSquadManager(PlayerController);
    if (!SquadManager)
    {
        return 0;
    }

    const TArray<AMACharacter*> Selected = GetSelectedAgents(PlayerController);
    TSet<UMASquad*> SquadsToDisband;
    for (AMACharacter* Agent : Selected)
    {
        if (Agent && Agent->CurrentSquad)
        {
            SquadsToDisband.Add(Agent->CurrentSquad);
        }
    }

    int32 DisbandedCount = 0;
    for (UMASquad* Squad : SquadsToDisband)
    {
        if (SquadManager->DisbandSquad(Squad))
        {
            DisbandedCount++;
        }
    }

    return DisbandedCount;
}

bool FMAInteractionRuntimeAdapter::CanEnterEditMode(const AMAPlayerController* PlayerController) const
{
    if (const UMAEditModeManager* EditModeManager = ResolveEditModeManager(PlayerController))
    {
        return EditModeManager->IsEditModeAvailable();
    }

    return false;
}

void FMAInteractionRuntimeAdapter::ToggleEditSelection(AMAPlayerController* PlayerController, AActor* HitActor) const
{
    if (!HitActor)
    {
        return;
    }

    if (UMAEditModeManager* EditModeManager = ResolveEditModeManager(PlayerController))
    {
        if (AMAPointOfInterest* POI = Cast<AMAPointOfInterest>(HitActor))
        {
            if (EditModeManager->GetSelectedPOIs().Contains(POI))
            {
                EditModeManager->DeselectObject(POI);
            }
            else
            {
                EditModeManager->SelectPOI(POI);
            }
            return;
        }

        if (EditModeManager->GetSelectedActor() == HitActor)
        {
            EditModeManager->DeselectObject(HitActor);
        }
        else
        {
            EditModeManager->SelectActor(HitActor);
        }
    }
}

void FMAInteractionRuntimeAdapter::ClearEditSelection(AMAPlayerController* PlayerController) const
{
    if (UMAEditModeManager* EditModeManager = ResolveEditModeManager(PlayerController))
    {
        EditModeManager->ClearSelection();
    }
}

AMAPointOfInterest* FMAInteractionRuntimeAdapter::CreatePOI(AMAPlayerController* PlayerController, const FVector& Location) const
{
    if (UMAEditModeManager* EditModeManager = ResolveEditModeManager(PlayerController))
    {
        return EditModeManager->CreatePOI(Location);
    }

    return nullptr;
}

void FMAInteractionRuntimeAdapter::DestroyAllPOIs(AMAPlayerController* PlayerController) const
{
    if (UMAEditModeManager* EditModeManager = ResolveEditModeManager(PlayerController))
    {
        EditModeManager->DestroyAllPOIs();
    }
}

FVector FMAInteractionRuntimeAdapter::ProjectToGround(AMAPlayerController* PlayerController, const FVector& WorldLocation) const
{
    UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
    if (!World)
    {
        return WorldLocation;
    }

    FHitResult HitResult;
    const FVector TraceStart = WorldLocation + FVector(0.f, 0.f, 10000.f);
    const FVector TraceEnd = WorldLocation - FVector(0.f, 0.f, 10000.f);

    if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
    {
        return HitResult.Location;
    }

    return WorldLocation;
}
