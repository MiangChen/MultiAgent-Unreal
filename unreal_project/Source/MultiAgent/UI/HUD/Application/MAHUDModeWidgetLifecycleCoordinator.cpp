// Mode-widget show/hide/reset lifecycle coordination.

#include "MAHUDModeWidgetLifecycleCoordinator.h"

#include "../Runtime/MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../SceneEditing/Presentation/MAEditWidget.h"
#include "../../SceneEditing/Presentation/MAModifyWidget.h"

void FMAHUDModeWidgetLifecycleCoordinator::ShowModifyWidget(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        return;
    }

    HUD->UIManager->ShowWidget(EMAWidgetType::Modify, true);
    HUD->StartSceneLabelVisualization();
}

void FMAHUDModeWidgetLifecycleCoordinator::HideModifyWidget(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        return;
    }

    HUD->StopSceneLabelVisualization();
    HUD->UIManager->HideWidget(EMAWidgetType::Modify);
    ModifyWidgetStateCoordinator.ResetWidget(HUD->UIManager->GetModifyWidget());
}

void FMAHUDModeWidgetLifecycleCoordinator::ShowEditWidget(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        return;
    }

    HUD->UIManager->ShowWidget(EMAWidgetType::Edit, true);
}

void FMAHUDModeWidgetLifecycleCoordinator::HideEditWidget(AMAHUD* HUD) const
{
    if (!HUD || !HUD->UIManager)
    {
        return;
    }

    HUD->UIManager->HideWidget(EMAWidgetType::Edit);
    EditWidgetStateCoordinator.ResetWidget(HUD->UIManager->GetEditWidget());
}
