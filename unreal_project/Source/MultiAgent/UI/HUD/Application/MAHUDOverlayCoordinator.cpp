// MAHUDOverlayCoordinator.cpp
// HUD 叠加层与 EditMode 协调器实现

#include "MAHUDOverlayCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Mode/MAEditWidget.h"
#include "../../../Core/Manager/MAEditModeManager.h"
#include "../../../Core/Manager/MAPIPCameraManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Environment/Utils/MAGoalActor.h"
#include "../../../Environment/Utils/MAPointOfInterest.h"
#include "../../../Input/MAPlayerController.h"
#include "../MAHUDWidget.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDOverlayCoordinator, Log, All);

void FMAHUDOverlayCoordinator::LoadSceneGraphForVisualization(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    HUD->CachedSceneNodes.Empty();

    UGameInstance* GI = HUD->GetWorld() ? HUD->GetWorld()->GetGameInstance() : nullptr;
    if (!GI)
    {
        UE_LOG(LogMAHUDOverlayCoordinator, Warning, TEXT("LoadSceneGraphForVisualization: GameInstance not found"));
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        UE_LOG(LogMAHUDOverlayCoordinator, Warning, TEXT("LoadSceneGraphForVisualization: SceneGraphManager not found"));
        return;
    }

    HUD->CachedSceneNodes = SceneGraphManager->GetAllNodes();
    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("LoadSceneGraphForVisualization: Loaded %d nodes"), HUD->CachedSceneNodes.Num());
}

void FMAHUDOverlayCoordinator::DrawSceneLabels(AMAHUD* HUD) const
{
    if (!HUD || !HUD->bShowingSceneLabels)
    {
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        return;
    }

    for (const FMASceneGraphNode& Node : HUD->CachedSceneNodes)
    {
        const FString IdDisplay = Node.Id.IsEmpty() ? TEXT("N/A") : Node.Id;
        const FString LabelDisplay = Node.Label.IsEmpty() ? TEXT("N/A") : Node.Label;
        const FString DisplayText = FString::Printf(TEXT("id: %s\nLabel: %s"), *IdDisplay, *LabelDisplay);
        const FVector TextPosition = Node.Center + FVector(0.0f, 0.0f, 100.0f);
        DrawDebugString(World, TextPosition, DisplayText, nullptr, FColor::Green, 0.0f, false);
    }
}

void FMAHUDOverlayCoordinator::DrawPIPCameras(AMAHUD* HUD) const
{
    if (!HUD || !HUD->Canvas)
    {
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        return;
    }

    UMAPIPCameraManager* PIPManager = World->GetSubsystem<UMAPIPCameraManager>();
    if (!PIPManager || PIPManager->GetVisiblePIPCameraCount() == 0)
    {
        return;
    }

    const int32 ViewportWidth = HUD->Canvas->SizeX;
    const int32 ViewportHeight = HUD->Canvas->SizeY;
    PIPManager->DrawPIPCameras(HUD->Canvas, ViewportWidth, ViewportHeight);
}

void FMAHUDOverlayCoordinator::StartSceneLabelVisualization(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    if (HUD->bShowingSceneLabels)
    {
        LoadSceneGraphForVisualization(HUD);
        return;
    }

    HUD->bShowingSceneLabels = true;
    LoadSceneGraphForVisualization(HUD);
    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("StartSceneLabelVisualization: Started with %d nodes"), HUD->CachedSceneNodes.Num());
}

void FMAHUDOverlayCoordinator::StopSceneLabelVisualization(AMAHUD* HUD) const
{
    if (!HUD || !HUD->bShowingSceneLabels)
    {
        return;
    }

    HUD->bShowingSceneLabels = false;
    HUD->CachedSceneNodes.Empty();
    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("StopSceneLabelVisualization: Stopped"));
}

void FMAHUDOverlayCoordinator::BindEditModeManagerEvents(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        UE_LOG(LogMAHUDOverlayCoordinator, Warning, TEXT("BindEditModeManagerEvents: World not found"));
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        UE_LOG(LogMAHUDOverlayCoordinator, Warning, TEXT("BindEditModeManagerEvents: EditModeManager not found"));
        return;
    }

    if (!EditModeManager->OnSelectionChanged.IsAlreadyBound(HUD, &AMAHUD::OnEditModeSelectionChanged))
    {
        EditModeManager->OnSelectionChanged.AddDynamic(HUD, &AMAHUD::OnEditModeSelectionChanged);
    }

    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("BindEditModeManagerEvents: Bound to EditModeManager"));
}

void FMAHUDOverlayCoordinator::BindEditWidgetDelegates(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    UMAEditWidget* EditWidget = HUD->UIManager ? HUD->UIManager->GetEditWidget() : nullptr;
    if (!EditWidget)
    {
        UE_LOG(LogMAHUDOverlayCoordinator, Warning, TEXT("BindEditWidgetDelegates: EditWidget is null"));
        return;
    }

    if (!EditWidget->OnEditConfirmed.IsAlreadyBound(HUD, &AMAHUD::OnEditConfirmed))
    {
        EditWidget->OnEditConfirmed.AddDynamic(HUD, &AMAHUD::OnEditConfirmed);
    }
    if (!EditWidget->OnDeleteActor.IsAlreadyBound(HUD, &AMAHUD::OnEditDeleteActor))
    {
        EditWidget->OnDeleteActor.AddDynamic(HUD, &AMAHUD::OnEditDeleteActor);
    }
    if (!EditWidget->OnCreateGoal.IsAlreadyBound(HUD, &AMAHUD::OnEditCreateGoal))
    {
        EditWidget->OnCreateGoal.AddDynamic(HUD, &AMAHUD::OnEditCreateGoal);
    }
    if (!EditWidget->OnCreateZone.IsAlreadyBound(HUD, &AMAHUD::OnEditCreateZone))
    {
        EditWidget->OnCreateZone.AddDynamic(HUD, &AMAHUD::OnEditCreateZone);
    }
    if (!EditWidget->OnAddPresetActor.IsAlreadyBound(HUD, &AMAHUD::OnEditAddPresetActor))
    {
        EditWidget->OnAddPresetActor.AddDynamic(HUD, &AMAHUD::OnEditAddPresetActor);
    }
    if (!EditWidget->OnDeletePOIs.IsAlreadyBound(HUD, &AMAHUD::OnEditDeletePOIs))
    {
        EditWidget->OnDeletePOIs.AddDynamic(HUD, &AMAHUD::OnEditDeletePOIs);
    }
    if (!EditWidget->OnSetAsGoal.IsAlreadyBound(HUD, &AMAHUD::OnEditSetAsGoal))
    {
        EditWidget->OnSetAsGoal.AddDynamic(HUD, &AMAHUD::OnEditSetAsGoal);
    }
    if (!EditWidget->OnUnsetAsGoal.IsAlreadyBound(HUD, &AMAHUD::OnEditUnsetAsGoal))
    {
        EditWidget->OnUnsetAsGoal.AddDynamic(HUD, &AMAHUD::OnEditUnsetAsGoal);
    }

    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("BindEditWidgetDelegates: Bound all EditWidget delegates"));
}

void FMAHUDOverlayCoordinator::DrawEditModeIndicator(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    AMAPlayerController* MAPC = HUD->GetMAPlayerController();
    const bool bInEditMode = MAPC && MAPC->CurrentMouseMode == EMAMouseMode::Edit;

    UMAHUDWidget* HUDWidget = HUD->UIManager ? HUD->UIManager->GetHUDWidget() : nullptr;
    if (HUDWidget)
    {
        HUDWidget->SetEditModeVisible(bInEditMode);
    }

    if (!bInEditMode)
    {
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    TArray<FString> POIInfos;
    const TArray<AMAPointOfInterest*> POIs = EditModeManager->GetAllPOIs();
    for (int32 i = 0; i < POIs.Num(); ++i)
    {
        if (POIs[i])
        {
            const FVector Loc = POIs[i]->GetActorLocation();
            POIInfos.Add(FString::Printf(TEXT("[%d](%.0f, %.0f, %.0f)"), i + 1, Loc.X, Loc.Y, Loc.Z));
        }
    }

    TArray<FString> GoalInfos;
    UGameInstance* GI = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        return;
    }

    const TArray<FMASceneGraphNode> AllGoals = SceneGraphManager->GetAllGoals();
    for (const FMASceneGraphNode& GoalNode : AllGoals)
    {
        FString Label = GoalNode.Label;
        if (Label.IsEmpty())
        {
            Label = GoalNode.Id;
        }

        AMAGoalActor* GoalActor = EditModeManager->GetGoalActorByNodeId(GoalNode.Id);
        if (GoalActor)
        {
            const FVector Loc = GoalActor->GetActorLocation();
            GoalInfos.Add(FString::Printf(TEXT("[%s](%.0f, %.0f, %.0f)"), *Label, Loc.X, Loc.Y, Loc.Z));
        }
        else
        {
            GoalInfos.Add(FString::Printf(TEXT("[%s]"), *Label));
        }
    }

    TArray<FString> ZoneInfos;
    const TArray<FMASceneGraphNode> AllZones = SceneGraphManager->GetAllZones();
    for (const FMASceneGraphNode& ZoneNode : AllZones)
    {
        FString Label = ZoneNode.Label;
        if (Label.IsEmpty())
        {
            Label = ZoneNode.Id;
        }
        ZoneInfos.Add(FString::Printf(TEXT("[%s]"), *Label));
    }

    if (HUDWidget)
    {
        HUDWidget->UpdatePOIList(POIInfos);
        HUDWidget->UpdateGoalList(GoalInfos);
        HUDWidget->UpdateZoneList(ZoneInfos);
    }
}

void FMAHUDOverlayCoordinator::HandleEditModeSelectionChanged(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("OnEditModeSelectionChanged"));

    UMAEditWidget* EditWidget = HUD->UIManager ? HUD->UIManager->GetEditWidget() : nullptr;
    if (!EditWidget)
    {
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    if (EditModeManager->HasSelectedActor())
    {
        EditWidget->SetSelectedActor(EditModeManager->GetSelectedActor());
    }
    else if (EditModeManager->HasSelectedPOIs())
    {
        EditWidget->SetSelectedPOIs(EditModeManager->GetSelectedPOIs());
    }
    else
    {
        EditWidget->ClearSelection();
    }
}

void FMAHUDOverlayCoordinator::HandleTempSceneGraphChanged(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("OnTempSceneGraphChanged"));

    UMAEditWidget* EditWidget = HUD->UIManager ? HUD->UIManager->GetEditWidget() : nullptr;
    if (EditWidget)
    {
        EditWidget->RefreshSceneGraphPreview();
    }
}
