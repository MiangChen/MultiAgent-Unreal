// HUD overlay and edit-mode coordination.

#include "MAHUDOverlayCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Mode/MAEditWidget.h"
#include "../../../Core/Manager/MAEditModeManager.h"
#include "../../../Core/Manager/MAPIPCameraManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Input/MAPlayerController.h"
#include "../MAHUDWidget.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDOverlayCoordinator, Log, All);

namespace
{
    void ClearEditModeIndicatorLists(UMAHUDWidget* HUDWidget)
    {
        if (!HUDWidget)
        {
            return;
        }

        HUDWidget->UpdatePOIList({});
        HUDWidget->UpdateGoalList({});
        HUDWidget->UpdateZoneList({});
    }
}

UMAEditWidget* FMAHUDOverlayCoordinator::ResolveEditWidget(AMAHUD* HUD) const
{
    return HUD && HUD->UIManager ? HUD->UIManager->GetEditWidget() : nullptr;
}

void FMAHUDOverlayCoordinator::BindEditWidgetDelegates(AMAHUD* HUD, UMAEditWidget* EditWidget) const
{
    if (!HUD || !EditWidget)
    {
        return;
    }

    if (!EditWidget->OnEditConfirmRequested.IsAlreadyBound(HUD, &AMAHUD::OnEditConfirmRequested))
    {
        EditWidget->OnEditConfirmRequested.AddDynamic(HUD, &AMAHUD::OnEditConfirmRequested);
    }
    if (!EditWidget->OnDeleteActorRequested.IsAlreadyBound(HUD, &AMAHUD::OnEditDeleteRequested))
    {
        EditWidget->OnDeleteActorRequested.AddDynamic(HUD, &AMAHUD::OnEditDeleteRequested);
    }
    if (!EditWidget->OnCreateGoalRequested.IsAlreadyBound(HUD, &AMAHUD::OnEditCreateGoalRequested))
    {
        EditWidget->OnCreateGoalRequested.AddDynamic(HUD, &AMAHUD::OnEditCreateGoalRequested);
    }
    if (!EditWidget->OnCreateZoneRequested.IsAlreadyBound(HUD, &AMAHUD::OnEditCreateZoneRequested))
    {
        EditWidget->OnCreateZoneRequested.AddDynamic(HUD, &AMAHUD::OnEditCreateZoneRequested);
    }
    if (!EditWidget->OnAddPresetActorRequested.IsAlreadyBound(HUD, &AMAHUD::OnEditAddPresetActorRequested))
    {
        EditWidget->OnAddPresetActorRequested.AddDynamic(HUD, &AMAHUD::OnEditAddPresetActorRequested);
    }
    if (!EditWidget->OnDeletePOIsRequested.IsAlreadyBound(HUD, &AMAHUD::OnEditDeletePOIsRequested))
    {
        EditWidget->OnDeletePOIsRequested.AddDynamic(HUD, &AMAHUD::OnEditDeletePOIsRequested);
    }
    if (!EditWidget->OnSetAsGoalRequested.IsAlreadyBound(HUD, &AMAHUD::OnEditSetAsGoalRequested))
    {
        EditWidget->OnSetAsGoalRequested.AddDynamic(HUD, &AMAHUD::OnEditSetAsGoalRequested);
    }
    if (!EditWidget->OnUnsetAsGoalRequested.IsAlreadyBound(HUD, &AMAHUD::OnEditUnsetAsGoalRequested))
    {
        EditWidget->OnUnsetAsGoalRequested.AddDynamic(HUD, &AMAHUD::OnEditUnsetAsGoalRequested);
    }
    if (!EditWidget->OnNodeSwitchRequested.IsAlreadyBound(HUD, &AMAHUD::OnEditNodeSwitchRequested))
    {
        EditWidget->OnNodeSwitchRequested.AddDynamic(HUD, &AMAHUD::OnEditNodeSwitchRequested);
    }
}

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

void FMAHUDOverlayCoordinator::BindEditWidgetDelegates(AMAHUD* HUD)
{
    if (!HUD)
    {
        return;
    }

    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    if (!EditWidget)
    {
        UE_LOG(LogMAHUDOverlayCoordinator, Warning, TEXT("BindEditWidgetDelegates: EditWidget is null"));
        return;
    }

    BindEditWidgetDelegates(HUD, EditWidget);

    EditWidgetCoordinator.RefreshFromEditModeSelection(HUD, EditWidget);
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
        ClearEditModeIndicatorLists(HUDWidget);
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        ClearEditModeIndicatorLists(HUDWidget);
        return;
    }

    FMAHUDEditModeIndicatorModel IndicatorModel;
    if (!EditModeIndicatorBuilder.Build(World, IndicatorModel))
    {
        ClearEditModeIndicatorLists(HUDWidget);
        return;
    }

    if (HUDWidget)
    {
        HUDWidget->UpdatePOIList(IndicatorModel.POIInfos);
        HUDWidget->UpdateGoalList(IndicatorModel.GoalInfos);
        HUDWidget->UpdateZoneList(IndicatorModel.ZoneInfos);
    }
}

void FMAHUDOverlayCoordinator::HandleEditModeSelectionChanged(AMAHUD* HUD)
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("OnEditModeSelectionChanged"));

    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    if (!EditWidget)
    {
        return;
    }

    EditWidgetCoordinator.RefreshFromEditModeSelection(HUD, EditWidget);
}

void FMAHUDOverlayCoordinator::HandleTempSceneGraphChanged(AMAHUD* HUD)
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("OnTempSceneGraphChanged"));

    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    if (EditWidget)
    {
        EditWidgetCoordinator.RefreshCurrentSelection(HUD, EditWidget);
    }
}

void FMAHUDOverlayCoordinator::HandleEditConfirmRequested(AMAHUD* HUD, const FString& JsonContent)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleConfirmRequested(HUD, EditWidget, JsonContent);
}

void FMAHUDOverlayCoordinator::HandleEditDeleteRequested(AMAHUD* HUD)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleDeleteActorRequested(HUD, EditWidget);
}

void FMAHUDOverlayCoordinator::HandleEditCreateGoalRequested(AMAHUD* HUD, const FString& Description)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleCreateGoalRequested(HUD, EditWidget, Description);
}

void FMAHUDOverlayCoordinator::HandleEditCreateZoneRequested(AMAHUD* HUD, const FString& Description)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleCreateZoneRequested(HUD, EditWidget, Description);
}

void FMAHUDOverlayCoordinator::HandleEditAddPresetActorRequested(AMAHUD* HUD, const FString& ActorType)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleAddPresetActorRequested(HUD, EditWidget, ActorType);
}

void FMAHUDOverlayCoordinator::HandleEditDeletePOIsRequested(AMAHUD* HUD)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleDeletePOIsRequested(HUD, EditWidget);
}

void FMAHUDOverlayCoordinator::HandleEditSetAsGoalRequested(AMAHUD* HUD)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleSetAsGoalRequested(HUD, EditWidget);
}

void FMAHUDOverlayCoordinator::HandleEditUnsetAsGoalRequested(AMAHUD* HUD)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleUnsetAsGoalRequested(HUD, EditWidget);
}

void FMAHUDOverlayCoordinator::HandleEditNodeSwitchRequested(AMAHUD* HUD, int32 NodeIndex)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleNodeSwitchRequested(HUD, EditWidget, NodeIndex);
}
