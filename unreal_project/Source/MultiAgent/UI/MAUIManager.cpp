// MAUIManager.cpp
// UI Manager 实现 - Widget 生命周期管理

#include "MAUIManager.h"
#include "MASimpleMainWidget.h"
#include "task_graph/MATaskPlannerWidget.h"
#include "skill_list/MASkillAllocationViewer.h"
#include "tools/MADirectControlIndicator.h"
#include "mode/MAEmergencyWidget.h"
#include "mode/MAModifyWidget.h"
#include "mode/MAEditWidget.h"
#include "mode/MASceneListWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"

DEFINE_LOG_CATEGORY(LogMAUIManager);

//=============================================================================
// 初始化
//=============================================================================

void UMAUIManager::Initialize(APlayerController* InOwningPC)
{
    if (!InOwningPC)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("Initialize: PlayerController is null"));
        return;
    }

    OwningPC = InOwningPC;
    UE_LOG(LogMAUIManager, Log, TEXT("UIManager initialized"));
}

void UMAUIManager::CreateAllWidgets()
{
    if (!OwningPC)
    {
        UE_LOG(LogMAUIManager, Error, TEXT("CreateAllWidgets: OwningPC is null, call Initialize() first"));
        return;
    }

    UE_LOG(LogMAUIManager, Log, TEXT("Creating all widgets..."));

    // 创建 TaskPlannerWidget (任务规划工作台)
    TaskPlannerWidget = CreateWidget<UMATaskPlannerWidget>(OwningPC, UMATaskPlannerWidget::StaticClass());
    if (TaskPlannerWidget)
    {
        TaskPlannerWidget->AddToViewport(GetWidgetZOrder(EMAWidgetType::TaskPlanner));
        TaskPlannerWidget->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::TaskPlanner, TaskPlannerWidget);
        
        // 尝试加载 Mock 数据
        if (TaskPlannerWidget->IsMockMode())
        {
            TaskPlannerWidget->LoadMockData();
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("TaskPlannerWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create TaskPlannerWidget"));
    }


    // 创建 SkillAllocationViewer (技能分配查看器)
    SkillAllocationViewer = CreateWidget<UMASkillAllocationViewer>(OwningPC, UMASkillAllocationViewer::StaticClass());
    if (SkillAllocationViewer)
    {
        SkillAllocationViewer->AddToViewport(GetWidgetZOrder(EMAWidgetType::SkillAllocation));
        SkillAllocationViewer->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::SkillAllocation, SkillAllocationViewer);
        
        // 尝试加载 Mock 数据
        SkillAllocationViewer->LoadMockData();
        
        UE_LOG(LogMAUIManager, Log, TEXT("SkillAllocationViewer created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SkillAllocationViewer"));
    }

    // 创建 SimpleMainWidget (Legacy，保留向后兼容)
    SimpleMainWidget = CreateWidget<UMASimpleMainWidget>(OwningPC, UMASimpleMainWidget::StaticClass());
    if (SimpleMainWidget)
    {
        // 不添加到 Viewport，仅保留引用
        SimpleMainWidget->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::SimpleMain, SimpleMainWidget);
        UE_LOG(LogMAUIManager, Log, TEXT("SimpleMainWidget created (legacy, not added to viewport)"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SimpleMainWidget"));
    }

    // 创建 SemanticMapWidget (后续阶段)
    if (SemanticMapWidgetClass)
    {
        SemanticMapWidget = CreateWidget<UUserWidget>(OwningPC, SemanticMapWidgetClass);
        if (SemanticMapWidget)
        {
            SemanticMapWidget->AddToViewport(GetWidgetZOrder(EMAWidgetType::SemanticMap));
            SemanticMapWidget->SetVisibility(ESlateVisibility::Collapsed);
            Widgets.Add(EMAWidgetType::SemanticMap, SemanticMapWidget);
            UE_LOG(LogMAUIManager, Log, TEXT("SemanticMapWidget created"));
        }
    }

    // 创建 DirectControlIndicator
    DirectControlIndicator = CreateWidget<UMADirectControlIndicator>(OwningPC, UMADirectControlIndicator::StaticClass());
    if (DirectControlIndicator)
    {
        DirectControlIndicator->AddToViewport(GetWidgetZOrder(EMAWidgetType::DirectControl));
        DirectControlIndicator->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::DirectControl, DirectControlIndicator);
        UE_LOG(LogMAUIManager, Log, TEXT("DirectControlIndicator created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create DirectControlIndicator"));
    }

    // 创建 EmergencyWidget
    EmergencyWidget = CreateWidget<UMAEmergencyWidget>(OwningPC, UMAEmergencyWidget::StaticClass());
    if (EmergencyWidget)
    {
        EmergencyWidget->AddToViewport(GetWidgetZOrder(EMAWidgetType::Emergency));
        EmergencyWidget->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::Emergency, EmergencyWidget);
        UE_LOG(LogMAUIManager, Log, TEXT("EmergencyWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create EmergencyWidget"));
    }

    // 创建 ModifyWidget
    ModifyWidget = CreateWidget<UMAModifyWidget>(OwningPC, UMAModifyWidget::StaticClass());
    if (ModifyWidget)
    {
        ModifyWidget->AddToViewport(GetWidgetZOrder(EMAWidgetType::Modify));
        ModifyWidget->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::Modify, ModifyWidget);
        UE_LOG(LogMAUIManager, Log, TEXT("ModifyWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create ModifyWidget"));
    }

    // 创建 EditWidget
    EditWidget = CreateWidget<UMAEditWidget>(OwningPC, UMAEditWidget::StaticClass());
    if (EditWidget)
    {
        EditWidget->AddToViewport(GetWidgetZOrder(EMAWidgetType::Edit));
        EditWidget->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::Edit, EditWidget);
        UE_LOG(LogMAUIManager, Log, TEXT("EditWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create EditWidget"));
    }

    // 创建 SceneListWidget
    SceneListWidget = CreateWidget<UMASceneListWidget>(OwningPC, UMASceneListWidget::StaticClass());
    if (SceneListWidget)
    {
        SceneListWidget->AddToViewport(GetWidgetZOrder(EMAWidgetType::SceneList));
        SceneListWidget->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::SceneList, SceneListWidget);
        UE_LOG(LogMAUIManager, Log, TEXT("SceneListWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SceneListWidget"));
    }

    UE_LOG(LogMAUIManager, Log, TEXT("All widgets created. Total: %d"), Widgets.Num());
}


//=============================================================================
// Widget 访问 - 通用接口
//=============================================================================

UUserWidget* UMAUIManager::GetWidget(EMAWidgetType Type) const
{
    if (Type == EMAWidgetType::None)
    {
        return nullptr;
    }

    const UUserWidget* const* FoundWidget = Widgets.Find(Type);
    if (FoundWidget && *FoundWidget)
    {
        return const_cast<UUserWidget*>(*FoundWidget);
    }

    return nullptr;
}

//=============================================================================
// Widget 访问 - 类型安全 Getter
//=============================================================================

UMATaskPlannerWidget* UMAUIManager::GetTaskPlannerWidget() const
{
    return TaskPlannerWidget;
}

UMASkillAllocationViewer* UMAUIManager::GetSkillAllocationViewer() const
{
    return SkillAllocationViewer;
}

UMASimpleMainWidget* UMAUIManager::GetSimpleMainWidget() const
{
    return SimpleMainWidget;
}

UMADirectControlIndicator* UMAUIManager::GetDirectControlIndicator() const
{
    return DirectControlIndicator;
}

UMAEmergencyWidget* UMAUIManager::GetEmergencyWidget() const
{
    return EmergencyWidget;
}

UMAModifyWidget* UMAUIManager::GetModifyWidget() const
{
    return ModifyWidget;
}

UMAEditWidget* UMAUIManager::GetEditWidget() const
{
    return EditWidget;
}

UMASceneListWidget* UMAUIManager::GetSceneListWidget() const
{
    return SceneListWidget;
}

//=============================================================================
// Widget 可见性控制
//=============================================================================

bool UMAUIManager::ShowWidget(EMAWidgetType Type, bool bSetFocus)
{
    UUserWidget* Widget = GetWidget(Type);
    if (!Widget)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("ShowWidget: Widget type %d not found"), static_cast<int32>(Type));
        return false;
    }

    if (Widget->IsVisible())
    {
        // 已经显示，只需设置焦点
        if (bSetFocus)
        {
            SetWidgetFocus(Type);
        }
        return true;
    }

    Widget->SetVisibility(ESlateVisibility::Visible);

    if (bSetFocus)
    {
        SetInputModeGameAndUI(Widget);
    }

    UE_LOG(LogMAUIManager, Log, TEXT("Widget type %d shown"), static_cast<int32>(Type));
    return true;
}

bool UMAUIManager::HideWidget(EMAWidgetType Type)
{
    UUserWidget* Widget = GetWidget(Type);
    if (!Widget)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("HideWidget: Widget type %d not found"), static_cast<int32>(Type));
        return false;
    }

    if (!Widget->IsVisible())
    {
        return true;  // 已经隐藏
    }

    Widget->SetVisibility(ESlateVisibility::Collapsed);
    SetInputModeGameOnly();

    UE_LOG(LogMAUIManager, Log, TEXT("Widget type %d hidden"), static_cast<int32>(Type));
    return true;
}

void UMAUIManager::ToggleWidget(EMAWidgetType Type)
{
    if (IsWidgetVisible(Type))
    {
        HideWidget(Type);
    }
    else
    {
        ShowWidget(Type);
    }
}

bool UMAUIManager::IsWidgetVisible(EMAWidgetType Type) const
{
    UUserWidget* Widget = GetWidget(Type);
    if (!Widget)
    {
        return false;
    }

    return Widget->IsVisible();
}

void UMAUIManager::SetWidgetFocus(EMAWidgetType Type)
{
    UUserWidget* Widget = GetWidget(Type);
    if (!Widget)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("SetWidgetFocus: Widget type %d not found"), static_cast<int32>(Type));
        return;
    }

    // 根据 Widget 类型设置特定焦点
    switch (Type)
    {
    case EMAWidgetType::TaskPlanner:
        if (TaskPlannerWidget)
        {
            TaskPlannerWidget->FocusJsonEditor();
        }
        break;

    case EMAWidgetType::SimpleMain:
        if (SimpleMainWidget)
        {
            SimpleMainWidget->FocusInputBox();
        }
        break;

    case EMAWidgetType::Emergency:
        if (EmergencyWidget)
        {
            EmergencyWidget->FocusInputBox();
        }
        break;

    default:
        // 其他 Widget 使用通用焦点设置
        SetInputModeGameAndUI(Widget);
        break;
    }
}


//=============================================================================
// 内部方法
//=============================================================================

int32 UMAUIManager::GetWidgetZOrder(EMAWidgetType Type) const
{
    // Widget Z-Order 配置
    // 数值越大，显示在越上层
    switch (Type)
    {
    case EMAWidgetType::SemanticMap:
        return 5;
    case EMAWidgetType::TaskPlanner:
        return 10;
    case EMAWidgetType::SkillAllocation:
        return 10;
    case EMAWidgetType::Modify:
        return 11;
    case EMAWidgetType::Edit:
        return 11;
    case EMAWidgetType::SceneList:
        return 11;
    case EMAWidgetType::Emergency:
        return 12;
    case EMAWidgetType::DirectControl:
        return 15;
    default:
        return 10;
    }
}

void UMAUIManager::SetInputModeGameAndUI(UUserWidget* Widget)
{
    if (!OwningPC || !Widget)
    {
        return;
    }

    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(Widget->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    OwningPC->SetInputMode(InputMode);
    OwningPC->SetShowMouseCursor(true);
}

void UMAUIManager::SetInputModeGameOnly()
{
    if (!OwningPC)
    {
        return;
    }

    FInputModeGameOnly InputMode;
    OwningPC->SetInputMode(InputMode);
    OwningPC->SetShowMouseCursor(true);  // 保持鼠标可见用于 RTS 操作
}
