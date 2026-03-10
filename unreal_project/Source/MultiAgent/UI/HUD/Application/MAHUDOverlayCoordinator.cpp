// HUD overlay and edit-mode coordination.

#include "MAHUDOverlayCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Mode/MAEditWidget.h"
#include "../../../Input/MAPlayerController.h"
#include "../MAHUDWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDOverlayCoordinator, Log, All);

namespace
{
    FMAHUDWidgetEditModeViewModel MakeClearedEditModeModel(const FMAHUDWidgetCoordinator& WidgetCoordinator, const bool bVisible)
    {
        return WidgetCoordinator.BuildEditModeViewModel(bVisible, {}, {}, {});
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

    if (!HUD->RuntimeLoadSceneGraphNodes(HUD->CachedSceneNodes))
    {
        UE_LOG(LogMAHUDOverlayCoordinator, Warning, TEXT("LoadSceneGraphForVisualization: Scene graph runtime unavailable"));
        return;
    }
    UE_LOG(LogMAHUDOverlayCoordinator, Log, TEXT("LoadSceneGraphForVisualization: Loaded %d nodes"), HUD->CachedSceneNodes.Num());
}

void FMAHUDOverlayCoordinator::DrawSceneLabels(AMAHUD* HUD) const
{
    if (!HUD || !HUD->bShowingSceneLabels)
    {
        return;
    }

    HUD->RuntimeDrawSceneLabels(HUD->CachedSceneNodes);
}

void FMAHUDOverlayCoordinator::DrawPIPCameras(AMAHUD* HUD) const
{
    HUD->RuntimeDrawPIPCameras();
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

    if (!HUD->RuntimeBindEditModeSelectionChanged())
    {
        UE_LOG(LogMAHUDOverlayCoordinator, Warning, TEXT("BindEditModeManagerEvents: runtime binding unavailable"));
        return;
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
    const bool bInEditMode = MAPC && MAPC->GetCurrentMouseMode() == EMAMouseMode::Edit;

    UMAHUDWidget* HUDWidget = HUD->UIManager ? HUD->UIManager->GetHUDWidget() : nullptr;
    if (HUDWidget)
    {
        HUDWidget->ApplyEditModeViewModel(HUD->WidgetCoordinator.BuildEditModeViewModel(bInEditMode, {}, {}, {}));
    }

    if (!bInEditMode)
    {
        if (HUDWidget)
        {
            HUDWidget->ApplyEditModeViewModel(MakeClearedEditModeModel(HUD->WidgetCoordinator, false));
        }
        return;
    }

    FMAHUDEditModeIndicatorModel IndicatorModel;
    if (!EditModeIndicatorBuilder.Build(HUD, IndicatorModel))
    {
        if (HUDWidget)
        {
            HUDWidget->ApplyEditModeViewModel(MakeClearedEditModeModel(HUD->WidgetCoordinator, true));
        }
        return;
    }

    if (HUDWidget)
    {
        HUDWidget->ApplyEditModeViewModel(
            HUD->WidgetCoordinator.BuildEditModeViewModel(true, IndicatorModel.POIInfos, IndicatorModel.GoalInfos, IndicatorModel.ZoneInfos));
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
    FMAEditWidgetActionRequest Request;
    if (EditWidgetCoordinator.HandleConfirmRequested(EditWidget, JsonContent, Request))
    {
        HUD->SceneEditCoordinator.HandleEditActionRequest(HUD, Request);
    }
}

void FMAHUDOverlayCoordinator::HandleEditDeleteRequested(AMAHUD* HUD)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    FMAEditWidgetActionRequest Request;
    if (EditWidgetCoordinator.HandleDeleteActorRequested(EditWidget, Request))
    {
        HUD->SceneEditCoordinator.HandleEditActionRequest(HUD, Request);
    }
}

void FMAHUDOverlayCoordinator::HandleEditCreateGoalRequested(AMAHUD* HUD, const FString& Description)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    FMAEditWidgetActionRequest Request;
    if (EditWidgetCoordinator.HandleCreateGoalRequested(EditWidget, Description, Request))
    {
        HUD->SceneEditCoordinator.HandleEditActionRequest(HUD, Request);
    }
}

void FMAHUDOverlayCoordinator::HandleEditCreateZoneRequested(AMAHUD* HUD, const FString& Description)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    FMAEditWidgetActionRequest Request;
    if (EditWidgetCoordinator.HandleCreateZoneRequested(EditWidget, Description, Request))
    {
        HUD->SceneEditCoordinator.HandleEditActionRequest(HUD, Request);
    }
}

void FMAHUDOverlayCoordinator::HandleEditAddPresetActorRequested(AMAHUD* HUD, const FString& ActorType)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    FMAEditWidgetActionRequest Request;
    if (EditWidgetCoordinator.HandleAddPresetActorRequested(EditWidget, ActorType, Request))
    {
        HUD->SceneEditCoordinator.HandleEditActionRequest(HUD, Request);
    }
}

void FMAHUDOverlayCoordinator::HandleEditDeletePOIsRequested(AMAHUD* HUD)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    FMAEditWidgetActionRequest Request;
    if (EditWidgetCoordinator.HandleDeletePOIsRequested(EditWidget, Request))
    {
        HUD->SceneEditCoordinator.HandleEditActionRequest(HUD, Request);
    }
}

void FMAHUDOverlayCoordinator::HandleEditSetAsGoalRequested(AMAHUD* HUD)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    FMAEditWidgetActionRequest Request;
    if (EditWidgetCoordinator.HandleSetAsGoalRequested(EditWidget, Request))
    {
        HUD->SceneEditCoordinator.HandleEditActionRequest(HUD, Request);
    }
}

void FMAHUDOverlayCoordinator::HandleEditUnsetAsGoalRequested(AMAHUD* HUD)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    FMAEditWidgetActionRequest Request;
    if (EditWidgetCoordinator.HandleUnsetAsGoalRequested(EditWidget, Request))
    {
        HUD->SceneEditCoordinator.HandleEditActionRequest(HUD, Request);
    }
}

void FMAHUDOverlayCoordinator::HandleEditNodeSwitchRequested(AMAHUD* HUD, int32 NodeIndex)
{
    UMAEditWidget* EditWidget = ResolveEditWidget(HUD);
    EditWidgetCoordinator.HandleNodeSwitchRequested(HUD, EditWidget, NodeIndex);
}
