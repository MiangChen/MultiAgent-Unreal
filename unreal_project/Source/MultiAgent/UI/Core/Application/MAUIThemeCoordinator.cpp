// MAUIThemeCoordinator.cpp
// UIManager Theme 协调器实现（L1 Application）

#include "MAUIThemeCoordinator.h"
#include "../MAUIManager.h"
#include "../MAUITheme.h"
#include "../../HUD/Presentation/MAHUDWidget.h"
#include "../../HUD/Presentation/MAMainHUDWidget.h"
#include "../../SceneEditing/Presentation/MAEditWidget.h"
#include "../../SceneEditing/Presentation/MAModifyWidget.h"
#include "../../SceneEditing/Presentation/MASceneListWidget.h"
#include "../../TaskGraph/Presentation/MATaskPlannerWidget.h"
#include "../../SkillAllocation/Presentation/MASkillAllocationViewer.h"
#include "../../SkillAllocation/Presentation/MAGanttCanvas.h"
#include "../../Components/Presentation/MADirectControlIndicator.h"
#include "../../TaskGraph/Presentation/MATaskGraphModal.h"
#include "../../SkillAllocation/Presentation/MASkillAllocationModal.h"
#include "../Modal/MADecisionModal.h"
#include "../../Components/Presentation/MASystemLogPanel.h"
#include "../../Components/Presentation/MAPreviewPanel.h"
#include "../../Components/Presentation/MAInstructionPanel.h"

void FMAUIThemeCoordinator::LoadTheme(UMAUIManager* UIManager, UMAUITheme* ThemeAsset) const
{
    if (!UIManager)
    {
        return;
    }

    if (ThemeAsset)
    {
        if (!ValidateTheme(ThemeAsset))
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("LoadTheme: Theme validation failed, using default theme"));
            if (!UIManager->GetDefaultThemeInternal())
            {
                UIManager->SetDefaultThemeInternal(NewObject<UMAUITheme>(UIManager));
            }
            UIManager->SetCurrentThemeInternal(UIManager->GetDefaultThemeInternal());
        }
        else
        {
            UIManager->SetCurrentThemeInternal(ThemeAsset);
            UE_LOG(LogMAUIManager, Log, TEXT("LoadTheme: Theme loaded successfully"));
        }
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("LoadTheme: ThemeAsset is null, using default theme"));
        if (!UIManager->GetDefaultThemeInternal())
        {
            UIManager->SetDefaultThemeInternal(NewObject<UMAUITheme>(UIManager));
        }
        UIManager->SetCurrentThemeInternal(UIManager->GetDefaultThemeInternal());
    }

    DistributeThemeToWidgets(UIManager);
}

bool FMAUIThemeCoordinator::ValidateTheme(UMAUITheme* InTheme) const
{
    if (!InTheme)
    {
        return false;
    }

    const bool bValid = InTheme->IsValid();
    if (!bValid)
    {
        if (!InTheme->HasValidColors())
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("ValidateTheme: Invalid colors (alpha <= 0)"));
        }
        if (!InTheme->HasValidLineHeight())
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("ValidateTheme: LineHeight out of valid range (1.5-1.6), current: %f"), InTheme->LineHeight);
        }
        if (!InTheme->HasValidCornerRadius())
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("ValidateTheme: CornerRadius must be positive, current: %f"), InTheme->CornerRadius);
        }
    }

    return bValid;
}

void FMAUIThemeCoordinator::DistributeThemeToWidgets(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    UMAUITheme* ThemeToApply = UIManager->GetTheme();
    if (!ThemeToApply)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("DistributeThemeToWidgets: No theme available"));
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("DistributeThemeToWidgets: Distributing theme to all widgets..."));

    int32 WidgetCount = 0;

    if (UMAHUDWidget* HUDWidget = UIManager->GetHUDWidget())
    {
        HUDWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget())
    {
        MainHUDWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMAEditWidget* EditWidget = UIManager->GetEditWidget())
    {
        EditWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMAModifyWidget* ModifyWidget = UIManager->GetModifyWidget())
    {
        ModifyWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMASceneListWidget* SceneListWidget = UIManager->GetSceneListWidget())
    {
        SceneListWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget())
    {
        TaskPlannerWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMASkillAllocationViewer* SkillAllocationViewer = UIManager->GetSkillAllocationViewer())
    {
        UMAGanttCanvas* GanttCanvas = SkillAllocationViewer->GetGanttCanvas();
        if (GanttCanvas)
        {
            GanttCanvas->ApplyTheme(ThemeToApply);
            WidgetCount++;
        }
    }

    if (UMADirectControlIndicator* DirectControlIndicator = UIManager->GetDirectControlIndicator())
    {
        DirectControlIndicator->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMATaskGraphModal* TaskGraphModal = UIManager->GetTaskGraphModal())
    {
        TaskGraphModal->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMASkillAllocationModal* SkillAllocationModal = UIManager->GetSkillAllocationModal())
    {
        SkillAllocationModal->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMADecisionModal* DecisionModal = UIManager->GetDecisionModal())
    {
        DecisionModal->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMASystemLogPanel* SystemLogPanel = UIManager->GetSystemLogPanel())
    {
        SystemLogPanel->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMAPreviewPanel* PreviewPanel = UIManager->GetPreviewPanel())
    {
        PreviewPanel->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    if (UMAInstructionPanel* InstructionPanel = UIManager->GetInstructionPanel())
    {
        InstructionPanel->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("DistributeThemeToWidgets: Theme applied to %d widgets"), WidgetCount);
    UIManager->OnThemeChanged.Broadcast(ThemeToApply);
}
