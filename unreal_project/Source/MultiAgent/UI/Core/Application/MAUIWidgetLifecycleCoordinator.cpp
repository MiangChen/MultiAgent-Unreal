// MAUIWidgetLifecycleCoordinator.cpp
// UIManager Widget 生命周期创建协调器实现（L1 Application）

#include "MAUIWidgetLifecycleCoordinator.h"
#include "../MAUIManager.h"
#include "../MAUITheme.h"
#include "../../HUD/MAHUDWidget.h"
#include "../../HUD/MAMainHUDWidget.h"
#include "../../TaskGraph/MATaskPlannerWidget.h"
#include "../../SkillAllocation/MASkillAllocationViewer.h"
#include "../../SkillAllocation/MAGanttCanvas.h"
#include "../../Components/MADirectControlIndicator.h"
#include "../../Mode/MAModifyWidget.h"
#include "../../Mode/MAEditWidget.h"
#include "../../Mode/MASceneListWidget.h"
#include "../../Components/MASystemLogPanel.h"
#include "../../Components/MAPreviewPanel.h"
#include "../../Components/MAInstructionPanel.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void FMAUIWidgetLifecycleCoordinator::CreateAllWidgets(UMAUIManager* UIManager) const
{
    APlayerController* OwningPC = UIManager ? UIManager->GetOwningPlayerController() : nullptr;
    if (!UIManager || !OwningPC)
    {
        UE_LOG(LogMAUIManager, Error, TEXT("CreateAllWidgets: OwningPC is null, call Initialize() first"));
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("Creating all widgets..."));

    CreateHUDWidgets(UIManager, OwningPC);
    UIManager->CreateModalWidgetsInternal();
    CreatePlannerWidgets(UIManager, OwningPC);
    CreateModeWidgets(UIManager, OwningPC);
    CreatePanelWidgets(UIManager, OwningPC);

    UE_LOG(LogMAUIManager, Log, TEXT("All widgets created. Total: %d"), UIManager->GetRegisteredWidgetCountInternal());
    BindRuntimeEventSources(UIManager);
}

void FMAUIWidgetLifecycleCoordinator::CreateHUDWidgets(UMAUIManager* UIManager, APlayerController* OwningPC) const
{
    UMAHUDWidget* HUDWidget = CreateWidget<UMAHUDWidget>(OwningPC, UMAHUDWidget::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::HUD, HUDWidget);
    if (HUDWidget)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::HUD, HUDWidget, ESlateVisibility::SelfHitTestInvisible);

        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            HUDWidget->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("HUDWidget created (always visible, ZOrder = -1)"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create HUDWidget"));
    }

    UMAMainHUDWidget* MainHUDWidget = CreateWidget<UMAMainHUDWidget>(OwningPC, UMAMainHUDWidget::StaticClass());
    UIManager->SetMainHUDWidgetInternal(MainHUDWidget);
    if (MainHUDWidget)
    {
        MainHUDWidget->AddToViewport(0);
        MainHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            MainHUDWidget->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("MainHUDWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create MainHUDWidget"));
    }
}

void FMAUIWidgetLifecycleCoordinator::CreatePlannerWidgets(UMAUIManager* UIManager, APlayerController* OwningPC) const
{
    UMATaskPlannerWidget* TaskPlannerWidget = CreateWidget<UMATaskPlannerWidget>(OwningPC, UMATaskPlannerWidget::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::TaskPlanner, TaskPlannerWidget);
    if (TaskPlannerWidget)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::TaskPlanner, TaskPlannerWidget, ESlateVisibility::Collapsed);
        if (TaskPlannerWidget->IsMockMode())
        {
            TaskPlannerWidget->LoadMockData();
        }
        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            TaskPlannerWidget->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("TaskPlannerWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create TaskPlannerWidget"));
    }

    UMASkillAllocationViewer* SkillAllocationViewer = CreateWidget<UMASkillAllocationViewer>(OwningPC, UMASkillAllocationViewer::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::SkillAllocation, SkillAllocationViewer);
    if (SkillAllocationViewer)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::SkillAllocation, SkillAllocationViewer, ESlateVisibility::Collapsed);
        SkillAllocationViewer->LoadMockData();

        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            if (UMAGanttCanvas* GanttCanvas = SkillAllocationViewer->GetGanttCanvas())
            {
                GanttCanvas->ApplyTheme(ThemeToApply);
            }
        }
        UE_LOG(LogMAUIManager, Log, TEXT("SkillAllocationViewer created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SkillAllocationViewer"));
    }

    const TSubclassOf<UUserWidget> SemanticMapWidgetClass = UIManager->GetSemanticMapWidgetClass();
    if (SemanticMapWidgetClass)
    {
        UUserWidget* SemanticMapWidget = CreateWidget<UUserWidget>(OwningPC, SemanticMapWidgetClass);
        UIManager->SetWidgetInstanceInternal(EMAWidgetType::SemanticMap, SemanticMapWidget);
        if (SemanticMapWidget)
        {
            UIManager->RegisterViewportWidgetInternal(EMAWidgetType::SemanticMap, SemanticMapWidget, ESlateVisibility::Collapsed);
            UE_LOG(LogMAUIManager, Log, TEXT("SemanticMapWidget created"));
        }
    }
}

void FMAUIWidgetLifecycleCoordinator::CreateModeWidgets(UMAUIManager* UIManager, APlayerController* OwningPC) const
{
    UMADirectControlIndicator* DirectControlIndicator = CreateWidget<UMADirectControlIndicator>(OwningPC, UMADirectControlIndicator::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::DirectControl, DirectControlIndicator);
    if (DirectControlIndicator)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::DirectControl, DirectControlIndicator, ESlateVisibility::Collapsed);
        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            DirectControlIndicator->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("DirectControlIndicator created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create DirectControlIndicator"));
    }

    UMAModifyWidget* ModifyWidget = CreateWidget<UMAModifyWidget>(OwningPC, UMAModifyWidget::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::Modify, ModifyWidget);
    if (ModifyWidget)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::Modify, ModifyWidget, ESlateVisibility::Collapsed);
        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            ModifyWidget->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("ModifyWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create ModifyWidget"));
    }

    UMAEditWidget* EditWidget = CreateWidget<UMAEditWidget>(OwningPC, UMAEditWidget::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::Edit, EditWidget);
    if (EditWidget)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::Edit, EditWidget, ESlateVisibility::Collapsed);
        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            EditWidget->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("EditWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create EditWidget"));
    }

    UMASceneListWidget* SceneListWidget = CreateWidget<UMASceneListWidget>(OwningPC, UMASceneListWidget::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::SceneList, SceneListWidget);
    if (SceneListWidget)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::SceneList, SceneListWidget, ESlateVisibility::Collapsed);
        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            SceneListWidget->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("SceneListWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SceneListWidget"));
    }
}

void FMAUIWidgetLifecycleCoordinator::CreatePanelWidgets(UMAUIManager* UIManager, APlayerController* OwningPC) const
{
    UMASystemLogPanel* SystemLogPanel = CreateWidget<UMASystemLogPanel>(OwningPC, UMASystemLogPanel::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::SystemLogPanel, SystemLogPanel);
    if (SystemLogPanel)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::SystemLogPanel, SystemLogPanel, ESlateVisibility::Collapsed);
        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            SystemLogPanel->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("SystemLogPanel created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SystemLogPanel"));
    }

    UMAPreviewPanel* PreviewPanel = CreateWidget<UMAPreviewPanel>(OwningPC, UMAPreviewPanel::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::PreviewPanel, PreviewPanel);
    if (PreviewPanel)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::PreviewPanel, PreviewPanel, ESlateVisibility::Collapsed);
        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            PreviewPanel->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("PreviewPanel created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create PreviewPanel"));
    }

    UMAInstructionPanel* InstructionPanel = CreateWidget<UMAInstructionPanel>(OwningPC, UMAInstructionPanel::StaticClass());
    UIManager->SetWidgetInstanceInternal(EMAWidgetType::InstructionPanel, InstructionPanel);
    if (InstructionPanel)
    {
        UIManager->RegisterViewportWidgetInternal(EMAWidgetType::InstructionPanel, InstructionPanel, ESlateVisibility::Collapsed);
        if (UMAUITheme* ThemeToApply = UIManager->GetTheme())
        {
            InstructionPanel->ApplyTheme(ThemeToApply);
        }
        UE_LOG(LogMAUIManager, Log, TEXT("InstructionPanel created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create InstructionPanel"));
    }
}

void FMAUIWidgetLifecycleCoordinator::BindRuntimeEventSources(UMAUIManager* UIManager) const
{
    UIManager->BindRuntimeEventSourcesInternal();
}
