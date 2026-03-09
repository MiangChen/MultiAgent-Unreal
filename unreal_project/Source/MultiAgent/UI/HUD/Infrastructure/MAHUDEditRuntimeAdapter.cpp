// L4 runtime bridge for HUD edit-mode, scene graph, and PIP queries.

#include "MAHUDEditRuntimeAdapter.h"

#include "../MAHUD.h"
#include "../../Mode/MASceneListWidget.h"
#include "../../../Core/Manager/MAEditModeManager.h"
#include "../../../Core/Manager/MAPIPCameraManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Environment/Utils/MAGoalActor.h"
#include "../../../Environment/Utils/MAPointOfInterest.h"
#include "../../../Environment/Utils/MAZoneActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UWorld* FMAHUDEditRuntimeAdapter::ResolveWorld(const AMAHUD* HUD) const
{
    return HUD ? HUD->GetWorld() : nullptr;
}

UMAEditModeManager* FMAHUDEditRuntimeAdapter::ResolveEditModeManager(const AMAHUD* HUD) const
{
    if (UWorld* World = ResolveWorld(HUD))
    {
        return World->GetSubsystem<UMAEditModeManager>();
    }

    return nullptr;
}

UMASceneGraphManager* FMAHUDEditRuntimeAdapter::ResolveSceneGraphManager(const AMAHUD* HUD) const
{
    if (UWorld* World = ResolveWorld(HUD))
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UMASceneGraphManager>();
        }
    }

    return nullptr;
}

UMAPIPCameraManager* FMAHUDEditRuntimeAdapter::ResolvePIPCameraManager(const AMAHUD* HUD) const
{
    if (UWorld* World = ResolveWorld(HUD))
    {
        return World->GetSubsystem<UMAPIPCameraManager>();
    }

    return nullptr;
}

bool FMAHUDEditRuntimeAdapter::LoadSceneGraphNodes(AMAHUD* HUD, TArray<FMASceneGraphNode>& OutNodes) const
{
    OutNodes.Reset();

    if (UMASceneGraphManager* SceneGraphManager = ResolveSceneGraphManager(HUD))
    {
        OutNodes = SceneGraphManager->GetAllNodes();
        return true;
    }

    return false;
}

void FMAHUDEditRuntimeAdapter::DrawSceneLabels(AMAHUD* HUD, const TArray<FMASceneGraphNode>& Nodes) const
{
    UWorld* World = ResolveWorld(HUD);
    if (!World)
    {
        return;
    }

    for (const FMASceneGraphNode& Node : Nodes)
    {
        const FString IdDisplay = Node.Id.IsEmpty() ? TEXT("N/A") : Node.Id;
        const FString LabelDisplay = Node.Label.IsEmpty() ? TEXT("N/A") : Node.Label;
        const FString DisplayText = FString::Printf(TEXT("id: %s\nLabel: %s"), *IdDisplay, *LabelDisplay);
        DrawDebugString(World, Node.Center + FVector(0.0f, 0.0f, 100.0f), DisplayText, nullptr, FColor::Green, 0.0f, false);
    }
}

void FMAHUDEditRuntimeAdapter::DrawPIPCameras(AMAHUD* HUD) const
{
    if (!HUD || !HUD->Canvas)
    {
        return;
    }

    UMAPIPCameraManager* PIPManager = ResolvePIPCameraManager(HUD);
    if (!PIPManager || PIPManager->GetVisiblePIPCameraCount() == 0)
    {
        return;
    }

    PIPManager->DrawPIPCameras(HUD->Canvas, HUD->Canvas->SizeX, HUD->Canvas->SizeY);
}

bool FMAHUDEditRuntimeAdapter::BindEditModeSelectionChanged(AMAHUD* HUD) const
{
    UMAEditModeManager* EditModeManager = ResolveEditModeManager(HUD);
    if (!HUD || !EditModeManager)
    {
        return false;
    }

    if (!EditModeManager->OnSelectionChanged.IsAlreadyBound(HUD, &AMAHUD::OnEditModeSelectionChanged))
    {
        EditModeManager->OnSelectionChanged.AddDynamic(HUD, &AMAHUD::OnEditModeSelectionChanged);
    }

    return true;
}

void FMAHUDEditRuntimeAdapter::AssignSceneListEditModeManager(AMAHUD* HUD, UMASceneListWidget* SceneListWidget) const
{
    if (!SceneListWidget)
    {
        return;
    }

    if (UMAEditModeManager* EditModeManager = ResolveEditModeManager(HUD))
    {
        SceneListWidget->SetEditModeManager(EditModeManager);
    }
}

bool FMAHUDEditRuntimeAdapter::BuildEditModeIndicatorModel(AMAHUD* HUD, FMAHUDEditModeIndicatorModel& OutModel) const
{
    OutModel = FMAHUDEditModeIndicatorModel();

    UMAEditModeManager* EditModeManager = ResolveEditModeManager(HUD);
    UMASceneGraphManager* SceneGraphManager = ResolveSceneGraphManager(HUD);
    if (!EditModeManager || !SceneGraphManager)
    {
        return false;
    }

    const TArray<AMAPointOfInterest*> AllPOIs = EditModeManager->GetAllPOIs();
    for (int32 Index = 0; Index < AllPOIs.Num(); ++Index)
    {
        if (AMAPointOfInterest* POI = AllPOIs[Index])
        {
            const FVector Location = POI->GetActorLocation();
            OutModel.POIInfos.Add(FString::Printf(TEXT("[%d](%.0f, %.0f, %.0f)"), Index + 1, Location.X, Location.Y, Location.Z));
        }
    }

    for (const FMASceneGraphNode& GoalNode : SceneGraphManager->GetAllGoals())
    {
        const FString Label = GoalNode.Label.IsEmpty() ? GoalNode.Id : GoalNode.Label;
        if (AMAGoalActor* GoalActor = EditModeManager->GetGoalActorByNodeId(GoalNode.Id))
        {
            const FVector Location = GoalActor->GetActorLocation();
            OutModel.GoalInfos.Add(FString::Printf(TEXT("[%s](%.0f, %.0f, %.0f)"), *Label, Location.X, Location.Y, Location.Z));
        }
        else
        {
            OutModel.GoalInfos.Add(FString::Printf(TEXT("[%s]"), *Label));
        }
    }

    for (const FMASceneGraphNode& ZoneNode : SceneGraphManager->GetAllZones())
    {
        const FString Label = ZoneNode.Label.IsEmpty() ? ZoneNode.Id : ZoneNode.Label;
        OutModel.ZoneInfos.Add(FString::Printf(TEXT("[%s]"), *Label));
    }

    return true;
}

bool FMAHUDEditRuntimeAdapter::SelectGoalById(AMAHUD* HUD, const FString& GoalId) const
{
    UMAEditModeManager* EditModeManager = ResolveEditModeManager(HUD);
    if (!EditModeManager)
    {
        return false;
    }

    if (AMAGoalActor* GoalActor = EditModeManager->GetGoalActorByNodeId(GoalId))
    {
        EditModeManager->SelectActor(GoalActor);
        return true;
    }

    UMASceneGraphManager* SceneGraphManager = ResolveSceneGraphManager(HUD);
    if (!SceneGraphManager)
    {
        return false;
    }

    const FMASceneGraphNode Node = SceneGraphManager->GetNodeById(GoalId);
    if (!Node.IsValid() || Node.Guid.IsEmpty())
    {
        return false;
    }

    if (AActor* FoundActor = EditModeManager->FindActorByGuid(Node.Guid))
    {
        EditModeManager->SelectActor(FoundActor);
        return true;
    }

    return false;
}

bool FMAHUDEditRuntimeAdapter::SelectZoneById(AMAHUD* HUD, const FString& ZoneId) const
{
    UMAEditModeManager* EditModeManager = ResolveEditModeManager(HUD);
    if (!EditModeManager)
    {
        return false;
    }

    if (AMAZoneActor* ZoneActor = EditModeManager->GetZoneActorByNodeId(ZoneId))
    {
        EditModeManager->SelectActor(ZoneActor);
        return true;
    }

    return false;
}
