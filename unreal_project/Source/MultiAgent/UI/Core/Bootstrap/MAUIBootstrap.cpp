#include "MAUIBootstrap.h"

#include "UI/HUD/MAHUD.h"
#include "UI/Core/MAUIManager.h"
#include "UI/HUD/MAHUDWidget.h"
#include "UI/TaskGraph/MATaskPlannerWidget.h"
#include "UI/TaskGraph/Bootstrap/MATaskGraphUIBootstrap.h"
#include "UI/SkillAllocation/MASkillAllocationViewer.h"
#include "UI/SkillAllocation/Bootstrap/MASkillAllocationUIBootstrap.h"
#include "UI/SceneEditing/MAModifyWidget.h"
#include "UI/SceneEditing/MAEditWidget.h"
#include "UI/SceneEditing/MASceneListWidget.h"
#include "UI/SceneEditing/Bootstrap/MASceneEditingBootstrap.h"
#include "UI/Components/MADirectControlIndicator.h"
#include "UI/Components/MASystemLogPanel.h"
#include "UI/Components/MAPreviewPanel.h"
#include "UI/Components/MAInstructionPanel.h"
#include "UI/Components/Bootstrap/MAComponentsBootstrap.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAUIBootstrap, Log, All);

UMAUIManager* FMAUIBootstrap::CreateAndInitializeUIManager(AMAHUD* HUD) const
{
    if (!HUD)
    {
        return nullptr;
    }

    APlayerController* PlayerController = HUD->GetOwningPlayerController();
    if (!PlayerController)
    {
        UE_LOG(LogMAUIBootstrap, Error, TEXT("CreateAndInitializeUIManager: No owning PlayerController"));
        return nullptr;
    }

    UMAUIManager* UIManager = NewObject<UMAUIManager>(HUD);
    if (!UIManager)
    {
        UE_LOG(LogMAUIBootstrap, Error, TEXT("CreateAndInitializeUIManager: Failed to create UIManager"));
        return nullptr;
    }

    UIManager->SemanticMapWidgetClass = HUD->SemanticMapWidgetClass;
    UIManager->Initialize(PlayerController);
    UIManager->LoadTheme(HUD->UIThemeAsset);
    UIManager->CreateAllWidgets();
    ConfigureContextWidgets(UIManager);
    return UIManager;
}

void FMAUIBootstrap::ConfigureContextWidgets(UMAUIManager* UIManager) const
{
    if (!UIManager)
    {
        return;
    }

    const FMATaskGraphUIBootstrap TaskGraphBootstrap;
    const FMASkillAllocationUIBootstrap SkillAllocationBootstrap;
    const FMASceneEditingBootstrap SceneEditingBootstrap;
    const FMAComponentsBootstrap ComponentsBootstrap;

    if (UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget())
    {
        TaskGraphBootstrap.ConfigureTaskPlannerWidget(TaskPlannerWidget, UIManager->GetTheme());
    }

    if (UMASkillAllocationViewer* SkillAllocationViewer = UIManager->GetSkillAllocationViewer())
    {
        SkillAllocationBootstrap.ConfigureSkillAllocationViewer(SkillAllocationViewer, UIManager->GetTheme());
    }

    if (UMAModifyWidget* ModifyWidget = UIManager->GetModifyWidget())
    {
        SceneEditingBootstrap.ConfigureModifyWidget(ModifyWidget, UIManager->GetTheme());
    }

    if (UMAEditWidget* EditWidget = UIManager->GetEditWidget())
    {
        SceneEditingBootstrap.ConfigureEditWidget(EditWidget, UIManager->GetTheme());
    }

    if (UMASceneListWidget* SceneListWidget = UIManager->GetSceneListWidget())
    {
        SceneEditingBootstrap.ConfigureSceneListWidget(SceneListWidget, UIManager->GetTheme());
    }

    if (UMADirectControlIndicator* DirectControlIndicator = UIManager->GetDirectControlIndicator())
    {
        ComponentsBootstrap.ConfigureDirectControlIndicator(DirectControlIndicator, UIManager->GetTheme());
    }

    if (UMASystemLogPanel* SystemLogPanel = UIManager->GetSystemLogPanel())
    {
        ComponentsBootstrap.ConfigureSystemLogPanel(SystemLogPanel, UIManager->GetTheme());
    }

    if (UMAPreviewPanel* PreviewPanel = UIManager->GetPreviewPanel())
    {
        ComponentsBootstrap.ConfigurePreviewPanel(PreviewPanel, UIManager->GetTheme());
    }

    if (UMAInstructionPanel* InstructionPanel = UIManager->GetInstructionPanel())
    {
        ComponentsBootstrap.ConfigureInstructionPanel(InstructionPanel, UIManager->GetTheme());
    }
}
