// MAHUDDelegateCoordinator.cpp
// HUD delegate binding coordination.

#include "MAHUDDelegateCoordinator.h"
#include "../Runtime/MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Components/Presentation/MAInstructionPanel.h"
#include "../../SceneEditing/Presentation/MAModifyWidget.h"
#include "../../SceneEditing/Presentation/MASceneListWidget.h"
#include "../../../Input/MAPlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDDelegateCoordinator, Log, All);

void FMAHUDDelegateCoordinator::BindWidgetDelegates(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        UE_LOG(LogMAHUDDelegateCoordinator, Warning, TEXT("BindWidgetDelegates: UIManager is null"));
        return;
    }

    UMAInstructionPanel* InstructionPanel = HUD->UIManager->GetInstructionPanel();
    if (InstructionPanel)
    {
        if (!InstructionPanel->OnCommandSubmitted.IsAlreadyBound(HUD, &AMAHUD::OnSimpleCommandSubmitted))
        {
            InstructionPanel->OnCommandSubmitted.AddDynamic(HUD, &AMAHUD::OnSimpleCommandSubmitted);
        }
        UE_LOG(LogMAHUDDelegateCoordinator, Log, TEXT("BindWidgetDelegates: Bound InstructionPanel delegates"));
    }

    UMAModifyWidget* ModifyWidget = HUD->UIManager->GetModifyWidget();
    if (ModifyWidget)
    {
        if (!ModifyWidget->OnModifyConfirmed.IsAlreadyBound(HUD, &AMAHUD::OnModifyConfirmed))
        {
            ModifyWidget->OnModifyConfirmed.AddDynamic(HUD, &AMAHUD::OnModifyConfirmed);
        }
        if (!ModifyWidget->OnMultiSelectModifyConfirmed.IsAlreadyBound(HUD, &AMAHUD::OnMultiSelectModifyConfirmed))
        {
            ModifyWidget->OnMultiSelectModifyConfirmed.AddDynamic(HUD, &AMAHUD::OnMultiSelectModifyConfirmed);
        }
        UE_LOG(LogMAHUDDelegateCoordinator, Log, TEXT("BindWidgetDelegates: Bound ModifyWidget delegates"));
    }

    UMASceneListWidget* SceneListWidget = HUD->UIManager->GetSceneListWidget();
    if (SceneListWidget)
    {
        if (!SceneListWidget->OnGoalItemClicked.IsAlreadyBound(HUD, &AMAHUD::OnSceneListGoalClicked))
        {
            SceneListWidget->OnGoalItemClicked.AddDynamic(HUD, &AMAHUD::OnSceneListGoalClicked);
        }
        if (!SceneListWidget->OnZoneItemClicked.IsAlreadyBound(HUD, &AMAHUD::OnSceneListZoneClicked))
        {
            SceneListWidget->OnZoneItemClicked.AddDynamic(HUD, &AMAHUD::OnSceneListZoneClicked);
        }
        UE_LOG(LogMAHUDDelegateCoordinator, Log, TEXT("BindWidgetDelegates: Bound SceneListWidget delegates"));
    }

    UE_LOG(LogMAHUDDelegateCoordinator, Log, TEXT("BindWidgetDelegates: All widget delegates bound"));
}

void FMAHUDDelegateCoordinator::BindControllerEvents(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return;
    }

    AMAPlayerController* MAPC = HUD->GetMAPlayerController();
    if (!MAPC)
    {
        UE_LOG(LogMAHUDDelegateCoordinator, Warning, TEXT("BindControllerEvents: MAPlayerController not found"));
        return;
    }

    MAPC->OnModifyActorSelected.AddDynamic(HUD, &AMAHUD::OnModifyActorSelected);
    UE_LOG(LogMAHUDDelegateCoordinator, Log, TEXT("BindControllerEvents: Bound OnModifyActorSelected delegate"));

    MAPC->OnModifyActorsSelected.AddDynamic(HUD, &AMAHUD::OnModifyActorsSelected);
    UE_LOG(LogMAHUDDelegateCoordinator, Log, TEXT("BindControllerEvents: Bound OnModifyActorsSelected delegate"));
    UE_LOG(LogMAHUDDelegateCoordinator, Log, TEXT("BindControllerEvents: Ready to bind when MAPlayerController delegates are available"));
}
