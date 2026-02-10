// MAUIManager.cpp
// UI Manager 实现 - Widget 生命周期管理
// 集成 HUD 状态管理器，处理 UI 状态转换和模态窗口管理

#include "MAUIManager.h"
#include "MAUITheme.h"
#include "../Legacy/MASimpleMainWidget.h"
#include "../HUD/MAHUDWidget.h"
#include "MAHUDStateManager.h"
#include "../HUD/MAMainHUDWidget.h"
#include "../Modal/MATaskGraphModal.h"
#include "../Modal/MASkillAllocationModal.h"
#include "../Modal/MAEmergencyModal.h"
#include "../Modal/MABaseModalWidget.h"
#include "../Components/MANotificationWidget.h"
#include "../TaskGraph/MATaskPlannerWidget.h"
#include "../SkillAllocation/MASkillAllocationViewer.h"
#include "../SkillAllocation/MAGanttCanvas.h"
#include "../Components/MADirectControlIndicator.h"
#include "../Mode/MAEmergencyWidget.h"
#include "../Mode/MAModifyWidget.h"
#include "../Mode/MAEditWidget.h"
#include "../Mode/MASceneListWidget.h"
// 右侧边栏拆分面板 (Requirements: 5.1)
#include "../Components/MASystemLogPanel.h"
#include "../Components/MAPreviewPanel.h"
#include "../Components/MAInstructionPanel.h"
#include "../../Core/Manager/MATempDataManager.h"
#include "../../Core/Manager/MAEmergencyManager.h"
#include "../../Core/Manager/MACommandManager.h"
#include "../../Core/Comm/MACommSubsystem.h"
#include "../../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "../../Agent/Character/MACharacter.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"

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
    
    // 初始化 HUD 状态管理器
    InitializeHUDStateManager();
    
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

    // 创建 HUDWidget (HUD 覆盖层，最底层，始终可见)
    HUDWidget = CreateWidget<UMAHUDWidget>(OwningPC, UMAHUDWidget::StaticClass());
    if (HUDWidget)
    {
        HUDWidget->AddToViewport(GetWidgetZOrder(EMAWidgetType::HUD));
        // HUDWidget 始终可见，不像其他 Widget 那样切换显示
        HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        Widgets.Add(EMAWidgetType::HUD, HUDWidget);
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            HUDWidget->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("HUDWidget created (always visible, ZOrder = -1)"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create HUDWidget"));
    }

    // 创建 MainHUDWidget (新的主 HUD，包含小地图、通知和右侧边栏)
    MainHUDWidget = CreateWidget<UMAMainHUDWidget>(OwningPC, UMAMainHUDWidget::StaticClass());
    if (MainHUDWidget)
    {
        MainHUDWidget->AddToViewport(0);  // 基础层
        MainHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            MainHUDWidget->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("MainHUDWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create MainHUDWidget"));
    }

    // 创建模态窗口
    CreateModalWidgets();

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
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            TaskPlannerWidget->ApplyTheme(ThemeToApply);
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
        
        // 应用主题到内部 GanttCanvas (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            UMAGanttCanvas* GanttCanvas = SkillAllocationViewer->GetGanttCanvas();
            if (GanttCanvas)
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
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            DirectControlIndicator->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("DirectControlIndicator created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create DirectControlIndicator"));
    }

    // 注意：旧的 EmergencyWidget 已被弃用，相机视图功能已移植到 EmergencyModal
    // 不再创建 EmergencyWidget，保留代码以便将来完全移除
    // EmergencyWidget = CreateWidget<UMAEmergencyWidget>(OwningPC, UMAEmergencyWidget::StaticClass());
    // if (EmergencyWidget)
    // {
    //     EmergencyWidget->AddToViewport(GetWidgetZOrder(EMAWidgetType::Emergency));
    //     EmergencyWidget->SetVisibility(ESlateVisibility::Collapsed);
    //     Widgets.Add(EMAWidgetType::Emergency, EmergencyWidget);
    //     UE_LOG(LogMAUIManager, Log, TEXT("EmergencyWidget created"));
    // }
    // else
    // {
    //     UE_LOG(LogMAUIManager, Error, TEXT("Failed to create EmergencyWidget"));
    // }
    EmergencyWidget = nullptr;
    UE_LOG(LogMAUIManager, Log, TEXT("EmergencyWidget deprecated - using EmergencyModal instead"));

    // 创建 ModifyWidget
    ModifyWidget = CreateWidget<UMAModifyWidget>(OwningPC, UMAModifyWidget::StaticClass());
    if (ModifyWidget)
    {
        ModifyWidget->AddToViewport(GetWidgetZOrder(EMAWidgetType::Modify));
        ModifyWidget->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::Modify, ModifyWidget);
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            ModifyWidget->ApplyTheme(ThemeToApply);
        }
        
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
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            EditWidget->ApplyTheme(ThemeToApply);
        }
        
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
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            SceneListWidget->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("SceneListWidget created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SceneListWidget"));
    }

    //=========================================================================
    // 右侧边栏拆分面板 (Requirements: 5.1)
    //=========================================================================

    // 创建 SystemLogPanel
    SystemLogPanel = CreateWidget<UMASystemLogPanel>(OwningPC, UMASystemLogPanel::StaticClass());
    if (SystemLogPanel)
    {
        SystemLogPanel->AddToViewport(GetWidgetZOrder(EMAWidgetType::SystemLogPanel));
        SystemLogPanel->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::SystemLogPanel, SystemLogPanel);
        
        // 应用主题 (Requirements: 7.1)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            SystemLogPanel->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("SystemLogPanel created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SystemLogPanel"));
    }

    // 创建 PreviewPanel
    PreviewPanel = CreateWidget<UMAPreviewPanel>(OwningPC, UMAPreviewPanel::StaticClass());
    if (PreviewPanel)
    {
        PreviewPanel->AddToViewport(GetWidgetZOrder(EMAWidgetType::PreviewPanel));
        PreviewPanel->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::PreviewPanel, PreviewPanel);
        
        // 应用主题 (Requirements: 7.2)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            PreviewPanel->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("PreviewPanel created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create PreviewPanel"));
    }

    // 创建 InstructionPanel
    InstructionPanel = CreateWidget<UMAInstructionPanel>(OwningPC, UMAInstructionPanel::StaticClass());
    if (InstructionPanel)
    {
        InstructionPanel->AddToViewport(GetWidgetZOrder(EMAWidgetType::InstructionPanel));
        InstructionPanel->SetVisibility(ESlateVisibility::Collapsed);
        Widgets.Add(EMAWidgetType::InstructionPanel, InstructionPanel);
        
        // 应用主题 (Requirements: 7.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            InstructionPanel->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("InstructionPanel created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create InstructionPanel"));
    }

    UE_LOG(LogMAUIManager, Log, TEXT("All widgets created. Total: %d"), Widgets.Num());

    // 绑定 TempDataManager 事件，以便在数据变化时更新预览组件
    BindTempDataManagerEvents();
    
    // 绑定 CommSubsystem 事件，以便接收后端消息
    BindCommSubsystemEvents();
    
    // 绑定 EmergencyManager 事件，以便接收紧急事件通知
    BindEmergencyManagerEvents();
    
    // 绑定 CommandManager 事件，以便接收暂停/恢复状态通知
    BindCommandManagerEvents();
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

UMAHUDWidget* UMAUIManager::GetHUDWidget() const
{
    return HUDWidget;
}

//=============================================================================
// Widget 访问 - 右侧边栏拆分面板 (Requirements: 5.2)
//=============================================================================

UMASystemLogPanel* UMAUIManager::GetSystemLogPanel() const
{
    return SystemLogPanel;
}

UMAPreviewPanel* UMAUIManager::GetPreviewPanel() const
{
    return PreviewPanel;
}

UMAInstructionPanel* UMAUIManager::GetInstructionPanel() const
{
    return InstructionPanel;
}

UMABaseModalWidget* UMAUIManager::GetModalByType(EMAModalType ModalType) const
{
    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        return TaskGraphModal;
    case EMAModalType::SkillAllocation:
        return SkillAllocationModal;
    case EMAModalType::Emergency:
        return EmergencyModal;
    default:
        return nullptr;
    }
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

    // 如果隐藏的是编辑界面 Widget，需要同步更新 HUDStateManager 状态
    // 这样可以确保状态机与实际 UI 显示状态一致
    if (HUDStateManager && HUDStateManager->IsModalActive())
    {
        // 检查是否是与当前模态状态关联的编辑界面
        EMAModalType ActiveModal = HUDStateManager->GetActiveModalType();
        bool bShouldReturnToNormal = false;

        switch (Type)
        {
        case EMAWidgetType::TaskPlanner:
            bShouldReturnToNormal = (ActiveModal == EMAModalType::TaskGraph);
            break;
        case EMAWidgetType::SkillAllocation:
            bShouldReturnToNormal = (ActiveModal == EMAModalType::SkillAllocation);
            break;
        case EMAWidgetType::Emergency:
            bShouldReturnToNormal = (ActiveModal == EMAModalType::Emergency);
            break;
        default:
            break;
        }

        if (bShouldReturnToNormal)
        {
            UE_LOG(LogMAUIManager, Log, TEXT("HideWidget: Returning HUDStateManager to NormalHUD (was editing %s)"),
                *UEnum::GetValueAsString(ActiveModal));
            HUDStateManager->ReturnToNormalHUD();
        }
    }

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

bool UMAUIManager::IsAnyFullscreenWidgetVisible() const
{
    // 检查全屏 Widget
    if (TaskPlannerWidget && TaskPlannerWidget->IsVisible())
    {
        return true;
    }
    
    if (SkillAllocationViewer && SkillAllocationViewer->IsVisible())
    {
        return true;
    }
    
    // 检查模态窗口
    if (TaskGraphModal && TaskGraphModal->IsVisible())
    {
        return true;
    }
    
    if (SkillAllocationModal && SkillAllocationModal->IsVisible())
    {
        return true;
    }
    
    if (EmergencyModal && EmergencyModal->IsVisible())
    {
        return true;
    }
    
    // 检查 Edit 模式 Widget
    if (EditWidget && EditWidget->IsVisible())
    {
        return true;
    }
    
    return false;
}

void UMAUIManager::NavigateFromViewerToSkillAllocationModal()
{
    UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: Starting navigation"));
    
    // 1. 隐藏 SkillAllocationViewer
    if (SkillAllocationViewer)
    {
        SkillAllocationViewer->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: SkillAllocationViewer hidden"));
    }
    
    // 2. 从 TempDataManager 加载数据到 SkillAllocationModal
    if (SkillAllocationModal)
    {
        if (UGameInstance* GameInstance = OwningPC ? OwningPC->GetGameInstance() : nullptr)
        {
            if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
            {
                FMASkillAllocationData Data;
                if (TempDataMgr->LoadSkillAllocation(Data))
                {
                    SkillAllocationModal->LoadSkillAllocation(Data);
                    UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: Data loaded into modal"));
                }
            }
        }
        
        // 3. 显示 SkillAllocationModal (只读模式，因为是从编辑界面返回查看)
        SkillAllocationModal->SetEditMode(false);
        SkillAllocationModal->SetVisibility(ESlateVisibility::Visible);
        SkillAllocationModal->PlayShowAnimation();
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillAllocationModal: SkillAllocationModal shown"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("NavigateFromViewerToSkillAllocationModal: SkillAllocationModal is null"));
    }
    
    // 4. 将 HUD 状态从 EditingModal 转换回 ReviewModal，以便 Edit 按钮可以再次工作
    if (HUDStateManager)
    {
        HUDStateManager->TransitionToState(EMAHUDState::ReviewModal, EMAModalType::SkillAllocation);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromViewerToSkillListModal: State transitioned back to ReviewModal"));
    }
}

void UMAUIManager::NavigateFromWorkbenchToTaskGraphModal()
{
    UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: Starting navigation"));
    
    // 1. 隐藏 TaskPlannerWidget
    if (TaskPlannerWidget)
    {
        TaskPlannerWidget->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: TaskPlannerWidget hidden"));
    }
    
    // 2. 从 TempDataManager 加载数据到 TaskGraphModal
    if (TaskGraphModal)
    {
        if (UGameInstance* GameInstance = OwningPC ? OwningPC->GetGameInstance() : nullptr)
        {
            if (UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>())
            {
                FMATaskGraphData Data;
                if (TempDataMgr->LoadTaskGraph(Data))
                {
                    TaskGraphModal->LoadTaskGraph(Data);
                    UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: Data loaded into modal"));
                }
            }
        }
        
        // 3. 显示 TaskGraphModal (只读模式，因为是从编辑界面返回查看)
        TaskGraphModal->SetEditMode(false);
        TaskGraphModal->SetVisibility(ESlateVisibility::Visible);
        TaskGraphModal->PlayShowAnimation();
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: TaskGraphModal shown"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("NavigateFromWorkbenchToTaskGraphModal: TaskGraphModal is null"));
    }
    
    // 4. 将 HUD 状态从 EditingModal 转换回 ReviewModal，以便 Edit 按钮可以再次工作
    if (HUDStateManager)
    {
        HUDStateManager->TransitionToState(EMAHUDState::ReviewModal, EMAModalType::TaskGraph);
        UE_LOG(LogMAUIManager, Log, TEXT("NavigateFromWorkbenchToTaskGraphModal: State transitioned back to ReviewModal"));
    }
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
    // HUD 使用 -1，确保在所有其他 Widget 之下
    // MiniMap 使用 1 (在 MAMiniMapManager 中设置)
    switch (Type)
    {
    case EMAWidgetType::HUD:
        return -1;  // 最底层，在所有其他 Widget 之下
    case EMAWidgetType::SemanticMap:
        return 5;
    case EMAWidgetType::Modify:
        return 10;
    case EMAWidgetType::Edit:
        return 10;
    case EMAWidgetType::SceneList:
        return 10;
    case EMAWidgetType::TaskPlanner:
        return 11;
    case EMAWidgetType::SkillAllocation:
        return 11;
    case EMAWidgetType::Emergency:
        return 12;
    case EMAWidgetType::DirectControl:
        return 15;
    // 右侧边栏拆分面板 - 使用较低的 Z-Order，不遮挡其他 Widget
    case EMAWidgetType::SystemLogPanel:
        return 5;
    case EMAWidgetType::PreviewPanel:
        return 5;
    case EMAWidgetType::InstructionPanel:
        return 5;
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

    // 先清除任何 Widget 的焦点
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
    }

    FInputModeGameOnly InputMode;
    OwningPC->SetInputMode(InputMode);
    OwningPC->SetShowMouseCursor(true);  // 保持鼠标可见用于 RTS 操作
    
    // 确保视口获得焦点
    if (UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport())
    {
        if (TSharedPtr<SViewport> ViewportWidget = ViewportClient->GetGameViewportWidget())
        {
            FSlateApplication::Get().SetKeyboardFocus(StaticCastSharedPtr<SWidget>(ViewportWidget));
        }
    }
}


//=============================================================================
// UI 主题
//=============================================================================

void UMAUIManager::LoadTheme(UMAUITheme* ThemeAsset)
{
    if (ThemeAsset)
    {
        // 验证主题完整性
        if (!ValidateTheme(ThemeAsset))
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("LoadTheme: Theme validation failed, using default theme"));
            // 创建默认主题
            if (!DefaultTheme)
            {
                DefaultTheme = NewObject<UMAUITheme>(this);
            }
            CurrentTheme = DefaultTheme;
        }
        else
        {
            CurrentTheme = ThemeAsset;
            UE_LOG(LogMAUIManager, Log, TEXT("LoadTheme: Theme loaded successfully"));
        }
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("LoadTheme: ThemeAsset is null, using default theme"));
        // 创建默认主题
        if (!DefaultTheme)
        {
            DefaultTheme = NewObject<UMAUITheme>(this);
        }
        CurrentTheme = DefaultTheme;
    }
    
    // 分发主题到所有已创建的 Widget (Requirements: 8.2)
    DistributeThemeToWidgets();
}

UMAUITheme* UMAUIManager::GetTheme() const
{
    if (CurrentTheme)
    {
        return CurrentTheme;
    }
    
    // 如果没有加载主题，返回默认主题
    if (DefaultTheme)
    {
        return DefaultTheme;
    }
    
    // 创建临时默认主题 (const_cast 用于延迟初始化)
    UMAUIManager* MutableThis = const_cast<UMAUIManager*>(this);
    MutableThis->DefaultTheme = NewObject<UMAUITheme>(MutableThis);
    return MutableThis->DefaultTheme;
}

bool UMAUIManager::ValidateTheme(UMAUITheme* InTheme) const
{
    if (!InTheme)
    {
        return false;
    }
    
    // 使用主题自身的验证方法
    bool bValid = InTheme->IsValid();
    
    if (!bValid)
    {
        // 详细日志
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

void UMAUIManager::DistributeThemeToWidgets()
{
    // Requirements: 8.1, 8.2
    // 分发主题到所有已注册的 Widget
    
    UMAUITheme* ThemeToApply = GetTheme();
    if (!ThemeToApply)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("DistributeThemeToWidgets: No theme available"));
        return;
    }
    
    UE_LOG(LogMAUIManager, Log, TEXT("DistributeThemeToWidgets: Distributing theme to all widgets..."));
    
    int32 WidgetCount = 0;
    
    //=========================================================================
    // HUD 层
    //=========================================================================
    
    if (HUDWidget)
    {
        HUDWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    if (MainHUDWidget)
    {
        MainHUDWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    //=========================================================================
    // Mode 层
    //=========================================================================
    
    if (EditWidget)
    {
        EditWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    if (ModifyWidget)
    {
        ModifyWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    if (SceneListWidget)
    {
        SceneListWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    //=========================================================================
    // TaskGraph 层
    //=========================================================================
    
    if (TaskPlannerWidget)
    {
        TaskPlannerWidget->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    //=========================================================================
    // SkillAllocation 层
    // Note: MASkillAllocationViewer 内部的 MAGanttCanvas 会在 Viewer 中处理主题
    //=========================================================================
    
    if (SkillAllocationViewer)
    {
        // MASkillAllocationViewer 内部包含 MAGanttCanvas
        // 如果 Viewer 有 ApplyTheme 方法，调用它
        // 否则直接调用内部 GanttCanvas 的 ApplyTheme
        UMAGanttCanvas* GanttCanvas = SkillAllocationViewer->GetGanttCanvas();
        if (GanttCanvas)
        {
            GanttCanvas->ApplyTheme(ThemeToApply);
            WidgetCount++;
        }
    }
    
    //=========================================================================
    // Components 层
    //=========================================================================
    
    if (DirectControlIndicator)
    {
        DirectControlIndicator->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    //=========================================================================
    // Modal 层
    //=========================================================================
    
    if (TaskGraphModal)
    {
        TaskGraphModal->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    if (SkillAllocationModal)
    {
        SkillAllocationModal->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    if (EmergencyModal)
    {
        EmergencyModal->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    //=========================================================================
    // 右侧边栏拆分面板 (Requirements: 7.4)
    //=========================================================================
    
    if (SystemLogPanel)
    {
        SystemLogPanel->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    if (PreviewPanel)
    {
        PreviewPanel->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    if (InstructionPanel)
    {
        InstructionPanel->ApplyTheme(ThemeToApply);
        WidgetCount++;
    }
    
    UE_LOG(LogMAUIManager, Log, TEXT("DistributeThemeToWidgets: Theme applied to %d widgets"), WidgetCount);
    
    // 广播主题变更事件 (Requirements: 8.4)
    OnThemeChanged.Broadcast(ThemeToApply);
}

//=============================================================================
// HUD 状态管理 (Requirements: 2.1, 2.3, 2.4, 2.5)
//=============================================================================

void UMAUIManager::InitializeHUDStateManager()
{
    // 创建 HUD 状态管理器
    HUDStateManager = NewObject<UMAHUDStateManager>(this);
    if (HUDStateManager)
    {
        // 绑定状态管理器委托
        BindStateManagerDelegates();
        UE_LOG(LogMAUIManager, Log, TEXT("HUDStateManager initialized"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create HUDStateManager"));
    }
}

void UMAUIManager::CreateModalWidgets()
{
    if (!OwningPC)
    {
        UE_LOG(LogMAUIManager, Error, TEXT("CreateModalWidgets: OwningPC is null"));
        return;
    }

    const int32 ModalZOrder = GetModalZOrder();

    // 创建任务图模态窗口
    TaskGraphModal = CreateWidget<UMATaskGraphModal>(OwningPC, UMATaskGraphModal::StaticClass());
    if (TaskGraphModal)
    {
        TaskGraphModal->AddToViewport(ModalZOrder);
        TaskGraphModal->SetVisibility(ESlateVisibility::Collapsed);
        TaskGraphModal->SetModalType(EMAModalType::TaskGraph);
        
        // 绑定模态按钮回调到 HUDStateManager
        if (HUDStateManager)
        {
            TaskGraphModal->OnConfirmClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalConfirm);
            TaskGraphModal->OnRejectClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalReject);
            TaskGraphModal->OnEditClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalEdit);
        }
        
        // 绑定隐藏动画完成回调，确保点击 ❌ 关闭按钮后状态同步
        TaskGraphModal->OnHideAnimationComplete.AddDynamic(this, &UMAUIManager::OnModalHideAnimationComplete);
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            TaskGraphModal->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("TaskGraphModal created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create TaskGraphModal"));
    }

    // 创建技能列表模态窗口
    SkillAllocationModal = CreateWidget<UMASkillAllocationModal>(OwningPC, UMASkillAllocationModal::StaticClass());
    if (SkillAllocationModal)
    {
        SkillAllocationModal->AddToViewport(ModalZOrder);
        SkillAllocationModal->SetVisibility(ESlateVisibility::Collapsed);
        SkillAllocationModal->SetModalType(EMAModalType::SkillAllocation);
        
        // 绑定模态按钮回调到 HUDStateManager
        if (HUDStateManager)
        {
            SkillAllocationModal->OnConfirmClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalConfirm);
            SkillAllocationModal->OnRejectClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalReject);
            SkillAllocationModal->OnEditClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalEdit);
        }
        
        // 绑定隐藏动画完成回调，确保点击 ❌ 关闭按钮后状态同步
        SkillAllocationModal->OnHideAnimationComplete.AddDynamic(this, &UMAUIManager::OnModalHideAnimationComplete);
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            SkillAllocationModal->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("SkillAllocationModal created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SkillAllocationModal"));
    }

    // 创建紧急事件模态窗口
    EmergencyModal = CreateWidget<UMAEmergencyModal>(OwningPC, UMAEmergencyModal::StaticClass());
    if (EmergencyModal)
    {
        EmergencyModal->AddToViewport(ModalZOrder);
        EmergencyModal->SetVisibility(ESlateVisibility::Collapsed);
        EmergencyModal->SetModalType(EMAModalType::Emergency);
        
        // 绑定模态按钮回调到 HUDStateManager
        if (HUDStateManager)
        {
            EmergencyModal->OnConfirmClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalConfirm);
            EmergencyModal->OnRejectClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalReject);
            // Emergency modal 不需要 Edit 按钮，因为它直接进入编辑模式
        }
        
        // 绑定紧急事件确认/拒绝回调，用于发送响应到后端
        // Requirements: 7.1, 7.4, 7.5
        EmergencyModal->OnEmergencyConfirmed.AddDynamic(this, &UMAUIManager::OnEmergencyModalConfirmed);
        EmergencyModal->OnEmergencyRejected.AddDynamic(this, &UMAUIManager::OnEmergencyModalRejected);
        
        // 绑定隐藏动画完成回调，确保点击 ❌ 关闭按钮后状态同步
        EmergencyModal->OnHideAnimationComplete.AddDynamic(this, &UMAUIManager::OnModalHideAnimationComplete);
        
        // 应用主题 (Requirements: 8.3)
        UMAUITheme* ThemeToApply = GetTheme();
        if (ThemeToApply)
        {
            EmergencyModal->ApplyTheme(ThemeToApply);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("EmergencyModal created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create EmergencyModal"));
    }
}

void UMAUIManager::BindStateManagerDelegates()
{
    if (!HUDStateManager)
    {
        return;
    }

    // 绑定状态变化委托
    HUDStateManager->OnStateChanged.AddDynamic(this, &UMAUIManager::OnHUDStateChanged);
    
    // 绑定通知接收委托
    HUDStateManager->OnNotificationReceived.AddDynamic(this, &UMAUIManager::OnNotificationReceived);
    
    // 绑定模态操作委托
    HUDStateManager->OnModalConfirmed.AddDynamic(this, &UMAUIManager::OnModalConfirmed);
    HUDStateManager->OnModalRejected.AddDynamic(this, &UMAUIManager::OnModalRejected);
    HUDStateManager->OnModalEditRequested.AddDynamic(this, &UMAUIManager::OnModalEditRequested);
    
    UE_LOG(LogMAUIManager, Log, TEXT("HUDStateManager delegates bound"));
}

void UMAUIManager::OnHUDStateChanged(EMAHUDState OldState, EMAHUDState NewState)
{
    UE_LOG(LogMAUIManager, Log, TEXT("HUD State changed: %s -> %s"),
        *UEnum::GetValueAsString(OldState),
        *UEnum::GetValueAsString(NewState));

    // 根据新状态显示/隐藏对应 Widget
    switch (NewState)
    {
    case EMAHUDState::NormalHUD:
        // 隐藏所有模态窗口
        HideCurrentModal();
        
        // 隐藏编辑界面 (TaskPlanner, SkillAllocation, Emergency)
        HideWidget(EMAWidgetType::TaskPlanner);
        HideWidget(EMAWidgetType::SkillAllocation);
        // 注意: EmergencyWidget 由 EmergencyManager 控制，不在这里隐藏
        
        // 隐藏通知
        if (MainHUDWidget && MainHUDWidget->GetNotification())
        {
            MainHUDWidget->GetNotification()->HideNotification();
        }
        
        // 恢复游戏输入模式
        SetInputModeGameOnly();
        break;

    case EMAHUDState::NotificationPending:
        // 通知已在 OnNotificationReceived 中显示
        // 保持游戏输入模式
        // 确保通知可见（可能是从 EditingModal 返回）
        if (MainHUDWidget && MainHUDWidget->GetNotification())
        {
            // 如果有待处理的紧急事件通知，确保显示
            if (HUDStateManager && HUDStateManager->GetPendingNotification() == EMANotificationType::EmergencyEvent)
            {
                MainHUDWidget->GetNotification()->ShowNotification(EMANotificationType::EmergencyEvent);
            }
        }
        break;

    case EMAHUDState::ReviewModal:
        // 隐藏通知 (用户已响应通知，打开了 modal)
        // 但对于 Emergency Modal，保留通知
        if (MainHUDWidget && MainHUDWidget->GetNotification())
        {
            if (HUDStateManager && HUDStateManager->GetActiveModalType() != EMAModalType::Emergency)
            {
                MainHUDWidget->GetNotification()->HideNotification();
            }
        }
        
        // 显示只读模态窗口
        if (HUDStateManager)
        {
            ShowModal(HUDStateManager->GetActiveModalType(), false);
        }
        break;

    case EMAHUDState::EditingModal:
        // 对于 Emergency Modal，不隐藏通知
        // 只有在用户点击 Confirm/Reject/Option 按钮时才隐藏通知
        if (MainHUDWidget && MainHUDWidget->GetNotification())
        {
            if (HUDStateManager && HUDStateManager->GetActiveModalType() != EMAModalType::Emergency)
            {
                MainHUDWidget->GetNotification()->HideNotification();
            }
        }
        
        // 编辑模式由 OnModalEditRequested 处理
        // 这里不需要额外操作，因为 OnModalEdit() 会触发 OnModalEditRequested
        // 如果是直接进入编辑模式（如突发事件），则显示对应的编辑界面
        if (HUDStateManager)
        {
            EMAModalType ModalType = HUDStateManager->GetActiveModalType();
            // 对于突发事件，显示 EmergencyModal（而不是 EmergencyWidget）
            // Requirements: 4.4, 4.5
            if (ModalType == EMAModalType::Emergency)
            {
                // 显示 EmergencyModal 并播放动画
                if (EmergencyModal)
                {
                    EmergencyModal->SetEditMode(true);
                    EmergencyModal->SetVisibility(ESlateVisibility::Visible);
                    EmergencyModal->PlayShowAnimation();
                    SetInputModeGameAndUI(EmergencyModal);
                    
                    // 确保相机源已设置 - 从 EmergencyManager 获取 Source Agent 的相机
                    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
                    if (World)
                    {
                        UMAEmergencyManager* EmergencyManager = World->GetSubsystem<UMAEmergencyManager>();
                        if (EmergencyManager)
                        {
                            AMACharacter* SourceAgent = EmergencyManager->GetSourceAgent();
                            if (SourceAgent)
                            {
                                UMACameraSensorComponent* Camera = SourceAgent->GetCameraSensor();
                                if (Camera)
                                {
                                    EmergencyModal->SetCameraSource(Camera);
                                    UE_LOG(LogMAUIManager, Log, TEXT("OnHUDStateChanged: Camera source set from SourceAgent %s"),
                                        *SourceAgent->AgentID);
                                }
                                else
                                {
                                    UE_LOG(LogMAUIManager, Warning, TEXT("OnHUDStateChanged: SourceAgent %s has no camera"),
                                        *SourceAgent->AgentID);
                                }
                            }
                            else
                            {
                                UE_LOG(LogMAUIManager, Warning, TEXT("OnHUDStateChanged: No SourceAgent available"));
                            }
                        }
                    }
                    
                    UE_LOG(LogMAUIManager, Log, TEXT("OnHUDStateChanged: Showing EmergencyModal for emergency event"));
                }
            }
        }
        break;
    }
}

void UMAUIManager::OnNotificationReceived(EMANotificationType Type)
{
    UE_LOG(LogMAUIManager, Log, TEXT("OnNotificationReceived: Type=%s"),
        *UEnum::GetValueAsString(Type));

    // 在 MainHUDWidget 的通知组件中显示通知
    if (MainHUDWidget)
    {
        UMANotificationWidget* NotificationWidget = MainHUDWidget->GetNotification();
        if (NotificationWidget)
        {
            UE_LOG(LogMAUIManager, Log, TEXT("OnNotificationReceived: Calling NotificationWidget->ShowNotification"));
            NotificationWidget->ShowNotification(Type);
        }
        else
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("OnNotificationReceived: NotificationWidget is null"));
        }
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnNotificationReceived: MainHUDWidget is null"));
    }
}

void UMAUIManager::OnModalConfirmed(EMAModalType ModalType)
{
    UE_LOG(LogMAUIManager, Log, TEXT("Modal confirmed: %s"),
        *UEnum::GetValueAsString(ModalType));

    // 模态确认后的处理由 MAHUD 或其他系统处理
    // 这里只负责 UI 状态管理
}

void UMAUIManager::OnModalRejected(EMAModalType ModalType)
{
    UE_LOG(LogMAUIManager, Log, TEXT("Modal rejected: %s"),
        *UEnum::GetValueAsString(ModalType));

    // 模态拒绝后的处理由 MAHUD 或其他系统处理
    // 这里只负责 UI 状态管理
}

void UMAUIManager::OnModalEditRequested(EMAModalType ModalType)
{
    UE_LOG(LogMAUIManager, Log, TEXT("Modal edit requested: %s"),
        *UEnum::GetValueAsString(ModalType));

    // 隐藏当前模态窗口
    HideCurrentModal();

    // 根据模态类型显示对应的编辑界面
    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        // 显示 TaskPlannerWidget (任务图编辑界面)
        ShowWidget(EMAWidgetType::TaskPlanner, true);
        UE_LOG(LogMAUIManager, Log, TEXT("Showing TaskPlannerWidget for editing"));
        break;

    case EMAModalType::SkillAllocation:
        // 显示 SkillAllocationViewer (技能列表编辑界面)
        ShowWidget(EMAWidgetType::SkillAllocation, true);
        UE_LOG(LogMAUIManager, Log, TEXT("Showing SkillAllocationViewer for editing"));
        break;

    case EMAModalType::Emergency:
        // 突发事件编辑 - 显示 EmergencyWidget
        ShowWidget(EMAWidgetType::Emergency, true);
        UE_LOG(LogMAUIManager, Log, TEXT("Showing EmergencyWidget for editing"));
        break;

    default:
        UE_LOG(LogMAUIManager, Warning, TEXT("Unknown modal type for edit: %s"),
            *UEnum::GetValueAsString(ModalType));
        break;
    }
}

void UMAUIManager::OnModalHideAnimationComplete()
{
    // 当模态窗口的隐藏动画完成时，需要判断是否应该重置状态
    // 
    // 有几种情况会触发模态隐藏动画：
    // 1. 用户点击 ❌ 关闭按钮 - 对于 Emergency Modal，只隐藏窗口，不结束事件
    // 2. 用户点击 Edit 按钮 - 此时状态已经是 EditingModal，不应该重置
    // 3. 用户点击 Confirm/Reject 按钮 - 此时事件已经在回调中处理
    //
    // 对于 Emergency Modal，点击 ❌ 关闭按钮时：
    // - 只隐藏窗口，不结束 emergency 状态
    // - 用户可以通过按 X 键重新打开窗口
    // - 只有点击 Confirm/Reject/Option 按钮才会结束 emergency 状态
    if (HUDStateManager && HUDStateManager->IsModalActive())
    {
        // 如果当前是编辑模式，说明是 Edit 流程触发的隐藏，不重置状态
        if (HUDStateManager->IsInEditMode())
        {
            UE_LOG(LogMAUIManager, Log, TEXT("OnModalHideAnimationComplete: In EditingModal state, skipping ReturnToNormalHUD"));
            return;
        }
        
        // 检查是否是 Emergency Modal
        EMAModalType ActiveModalType = HUDStateManager->GetActiveModalType();
        if (ActiveModalType == EMAModalType::Emergency)
        {
            // 对于 Emergency Modal，点击 ❌ 只隐藏窗口，不结束事件
            // 用户可以通过按 X 键重新打开窗口
            // 保留 PendingNotification 为 EmergencyEvent，这样用户可以重新打开
            UE_LOG(LogMAUIManager, Log, TEXT("OnModalHideAnimationComplete: Emergency Modal closed via X button, keeping emergency state and notification active"));
            
            // 转换到 NotificationPending 状态，保留通知
            // 不调用 ReturnToNormalHUD，因为那会清除通知
            HUDStateManager->TransitionToState(EMAHUDState::NotificationPending);
            return;
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("OnModalHideAnimationComplete: Returning HUDStateManager to NormalHUD"));
        HUDStateManager->ReturnToNormalHUD();
    }
}

void UMAUIManager::ShowModal(EMAModalType ModalType, bool bEditMode)
{
    // 先隐藏当前模态
    HideCurrentModal();

    // 获取对应的模态窗口
    UMABaseModalWidget* Modal = GetModalByType(ModalType);
    if (!Modal)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("ShowModal: Modal not found for type %s"),
            *UEnum::GetValueAsString(ModalType));
        return;
    }

    // 加载数据到模态窗口
    LoadDataIntoModal(ModalType);

    // 设置编辑模式
    Modal->SetEditMode(bEditMode);

    // 显示模态并播放动画
    Modal->SetVisibility(ESlateVisibility::Visible);
    Modal->PlayShowAnimation();

    // 设置输入模式为 UI 和游戏混合
    SetInputModeGameAndUI(Modal);

    UE_LOG(LogMAUIManager, Log, TEXT("Modal shown: %s (EditMode: %s)"),
        *UEnum::GetValueAsString(ModalType),
        bEditMode ? TEXT("true") : TEXT("false"));
}

void UMAUIManager::HideCurrentModal()
{
    // 隐藏所有模态窗口
    if (TaskGraphModal && TaskGraphModal->IsVisible())
    {
        TaskGraphModal->PlayHideAnimation();
        // 动画完成后会自动隐藏
    }

    if (SkillAllocationModal && SkillAllocationModal->IsVisible())
    {
        SkillAllocationModal->PlayHideAnimation();
    }

    if (EmergencyModal && EmergencyModal->IsVisible())
    {
        EmergencyModal->PlayHideAnimation();
    }
}

void UMAUIManager::LoadDataIntoModal(EMAModalType ModalType)
{
    // 获取 TempDataManager
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    UMATempDataManager* TempDataMgr = GameInstance ? GameInstance->GetSubsystem<UMATempDataManager>() : nullptr;

    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        if (TaskGraphModal)
        {
            // 只从 TempDataManager 加载数据，不使用 mock 数据
            if (TempDataMgr && TempDataMgr->TaskGraphFileExists())
            {
                FMATaskGraphData Data;
                if (TempDataMgr->LoadTaskGraph(Data))
                {
                    TaskGraphModal->LoadTaskGraph(Data);
                    UE_LOG(LogMAUIManager, Log, TEXT("TaskGraphModal: Loaded data from temp file (%d nodes, %d edges)"),
                        Data.Nodes.Num(), Data.Edges.Num());
                    return;
                }
            }
            
            // 如果没有数据，清空 Modal 并显示空状态（不加载 mock 数据）
            FMATaskGraphData EmptyData;
            TaskGraphModal->LoadTaskGraph(EmptyData);
            UE_LOG(LogMAUIManager, Log, TEXT("TaskGraphModal: No data available, showing empty modal (waiting for backend data)"));
        }
        break;

    case EMAModalType::SkillAllocation:
        if (SkillAllocationModal)
        {
            // 只从 TempDataManager 加载数据，不使用 mock 数据
            if (TempDataMgr && TempDataMgr->SkillAllocationFileExists())
            {
                FMASkillAllocationData Data;
                if (TempDataMgr->LoadSkillAllocation(Data))
                {
                    SkillAllocationModal->LoadSkillAllocation(Data);
                    UE_LOG(LogMAUIManager, Log, TEXT("SkillAllocationModal: Loaded data from temp file (%d time steps)"),
                        Data.Data.Num());
                    return;
                }
            }
            
            // 如果没有数据，清空 Modal 并显示空状态（不加载 mock 数据）
            FMASkillAllocationData EmptyData;
            SkillAllocationModal->LoadSkillAllocation(EmptyData);
            UE_LOG(LogMAUIManager, Log, TEXT("SkillAllocationModal: No data available, showing empty modal (waiting for backend data)"));
        }
        break;

    case EMAModalType::Emergency:
        // 紧急事件模态暂不需要预加载数据
        break;

    default:
        break;
    }
}

int32 UMAUIManager::GetModalZOrder() const
{
    // 模态窗口应该在所有其他 Widget 之上
    return 20;
}

//=============================================================================
// 通知处理 (Requirements: 4.1, 4.2, 4.3, 4.5)
//=============================================================================

void UMAUIManager::ShowNotification(EMANotificationType Type)
{
    if (HUDStateManager)
    {
        HUDStateManager->ShowNotification(Type);
    }
}

void UMAUIManager::DismissNotification()
{
    if (HUDStateManager)
    {
        HUDStateManager->DismissNotification();
    }
    
    // 同时隐藏通知 Widget
    if (MainHUDWidget)
    {
        UMANotificationWidget* NotificationWidget = MainHUDWidget->GetNotification();
        if (NotificationWidget)
        {
            NotificationWidget->HideNotification();
        }
    }
}

void UMAUIManager::DismissRequestUserCommandNotification()
{
    if (MainHUDWidget)
    {
        UMANotificationWidget* NotificationWidget = MainHUDWidget->GetNotification();
        if (NotificationWidget)
        {
            // 只关闭 RequestUserCommand 类型的通知
            if (NotificationWidget->GetCurrentNotificationType() == EMANotificationType::RequestUserCommand)
            {
                NotificationWidget->HideNotification();
                UE_LOG(LogMAUIManager, Log, TEXT("DismissRequestUserCommandNotification: RequestUserCommand notification dismissed"));
            }
        }
    }
}

EMAModalType UMAUIManager::GetModalTypeForNotification(EMANotificationType NotificationType) const
{
    switch (NotificationType)
    {
    case EMANotificationType::TaskGraphUpdate:
        return EMAModalType::TaskGraph;

    case EMANotificationType::SkillListUpdate:
        return EMAModalType::SkillAllocation;

    case EMANotificationType::EmergencyEvent:
        return EMAModalType::Emergency;

    default:
        return EMAModalType::None;
    }
}


//=============================================================================
// TempDataManager 事件绑定
//=============================================================================

void UMAUIManager::BindTempDataManagerEvents()
{
    // 获取 TempDataManager
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    
    if (!GameInstance)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindTempDataManagerEvents: GameInstance not available"));
        return;
    }
    
    UMATempDataManager* TempDataMgr = GameInstance->GetSubsystem<UMATempDataManager>();
    if (!TempDataMgr)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindTempDataManagerEvents: TempDataManager not available"));
        return;
    }
    
    // 绑定任务图数据变化事件
    if (!TempDataMgr->OnTaskGraphChanged.IsAlreadyBound(this, &UMAUIManager::OnTempTaskGraphChanged))
    {
        TempDataMgr->OnTaskGraphChanged.AddDynamic(this, &UMAUIManager::OnTempTaskGraphChanged);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Bound OnTaskGraphChanged event"));
    }
    
    // 绑定技能分配数据变化事件
    if (!TempDataMgr->OnSkillAllocationChanged.IsAlreadyBound(this, &UMAUIManager::OnTempSkillAllocationChanged))
    {
        TempDataMgr->OnSkillAllocationChanged.AddDynamic(this, &UMAUIManager::OnTempSkillAllocationChanged);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Bound OnSkillAllocationChanged event"));
    }
    
    // 绑定技能状态实时更新事件
    if (!TempDataMgr->OnSkillStatusUpdated.IsAlreadyBound(this, &UMAUIManager::OnSkillStatusUpdated))
    {
        TempDataMgr->OnSkillStatusUpdated.AddDynamic(this, &UMAUIManager::OnSkillStatusUpdated);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Bound OnSkillStatusUpdated event"));
    }
    
    // 只有在 unreal_project/data 目录下有临时数据文件时才加载
    // 不提前加载 mock 数据，等待后端发送数据后再显示
    FMATaskGraphData TaskGraphData;
    if (TempDataMgr->LoadTaskGraph(TaskGraphData) && TaskGraphData.Nodes.Num() > 0)
    {
        OnTempTaskGraphChanged(TaskGraphData);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Loaded existing task graph data (%d nodes)"), 
            TaskGraphData.Nodes.Num());
    }
    
    FMASkillAllocationData SkillAllocationData;
    if (TempDataMgr->LoadSkillAllocation(SkillAllocationData) && SkillAllocationData.Data.Num() > 0)
    {
        OnTempSkillAllocationChanged(SkillAllocationData);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Loaded existing skill allocation data (%d time steps)"), 
            SkillAllocationData.Data.Num());
    }
}

void UMAUIManager::OnTempTaskGraphChanged(const FMATaskGraphData& Data)
{
    UE_LOG(LogMAUIManager, Log, TEXT("OnTempTaskGraphChanged: Received task graph update (%d nodes, %d edges)"),
        Data.Nodes.Num(), Data.Edges.Num());
    
    // 更新 PreviewPanel 的任务图预览组件
    if (PreviewPanel)
    {
        PreviewPanel->UpdateTaskGraphPreview(Data);
    }
}

void UMAUIManager::OnTempSkillAllocationChanged(const FMASkillAllocationData& Data)
{
    UE_LOG(LogMAUIManager, Log, TEXT("OnTempSkillAllocationChanged: Received skill allocation update (%d time steps)"),
        Data.Data.Num());
    
    // 更新 PreviewPanel 的技能列表预览组件
    if (PreviewPanel)
    {
        PreviewPanel->UpdateSkillListPreview(Data);
    }
}

void UMAUIManager::OnSkillStatusUpdated(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    UE_LOG(LogMAUIManager, Log, TEXT("OnSkillStatusUpdated: TimeStep=%d, RobotId=%s, Status=%d"),
        TimeStep, *RobotId, static_cast<int32>(NewStatus));
    
    // 更新 PreviewPanel 的技能列表预览组件
    if (PreviewPanel)
    {
        UE_LOG(LogMAUIManager, Log, TEXT("OnSkillStatusUpdated: Forwarding to PreviewPanel"));
        PreviewPanel->UpdateSkillStatus(TimeStep, RobotId, NewStatus);
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnSkillStatusUpdated: PreviewPanel is null"));
    }
    
    // 更新 SkillAllocationModal 的甘特图
    if (SkillAllocationModal)
    {
        SkillAllocationModal->UpdateSkillStatus(TimeStep, RobotId, NewStatus);
    }
}

//=============================================================================
// CommSubsystem 事件绑定
//=============================================================================

void UMAUIManager::BindCommSubsystemEvents()
{
    // 获取 CommSubsystem
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    
    if (!GameInstance)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommSubsystemEvents: GameInstance not available"));
        return;
    }
    
    UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>();
    if (!CommSubsystem)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommSubsystemEvents: CommSubsystem not available"));
        return;
    }
    
    // 绑定索要用户指令请求委托
    if (!CommSubsystem->OnRequestUserCommandReceived.IsAlreadyBound(this, &UMAUIManager::OnRequestUserCommandReceived))
    {
        CommSubsystem->OnRequestUserCommandReceived.AddDynamic(this, &UMAUIManager::OnRequestUserCommandReceived);
        UE_LOG(LogMAUIManager, Log, TEXT("BindCommSubsystemEvents: Bound OnRequestUserCommandReceived"));
    }
    
    UE_LOG(LogMAUIManager, Log, TEXT("BindCommSubsystemEvents: CommSubsystem events bound"));
}

void UMAUIManager::OnRequestUserCommandReceived()
{
    UE_LOG(LogMAUIManager, Log, TEXT("OnRequestUserCommandReceived: Received request for user command"));
    
    // 显示索要用户指令通知
    ShowNotification(EMANotificationType::RequestUserCommand);
}

//=============================================================================
// CommandManager 事件绑定 (Requirements: 8.5, 8.6)
//=============================================================================

void UMAUIManager::BindCommandManagerEvents()
{
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    if (!World)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommandManagerEvents: World not available"));
        return;
    }
    
    UMACommandManager* CommandManager = World->GetSubsystem<UMACommandManager>();
    if (!CommandManager)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindCommandManagerEvents: CommandManager not available"));
        return;
    }
    
    // 绑定暂停状态变化委托 - Requirements: 8.5, 8.6
    if (!CommandManager->OnExecutionPauseStateChanged.IsAlreadyBound(this, &UMAUIManager::OnExecutionPauseStateChanged))
    {
        CommandManager->OnExecutionPauseStateChanged.AddDynamic(this, &UMAUIManager::OnExecutionPauseStateChanged);
        UE_LOG(LogMAUIManager, Log, TEXT("BindCommandManagerEvents: Bound OnExecutionPauseStateChanged"));
    }
    
    UE_LOG(LogMAUIManager, Log, TEXT("BindCommandManagerEvents: CommandManager events bound"));
}

void UMAUIManager::OnExecutionPauseStateChanged(bool bPaused)
{
    UE_LOG(LogMAUIManager, Log, TEXT("OnExecutionPauseStateChanged: bPaused=%s"), bPaused ? TEXT("true") : TEXT("false"));
    
    // 清除之前的恢复通知自动隐藏定时器
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    if (World)
    {
        World->GetTimerManager().ClearTimer(ResumeNotificationTimerHandle);
    }
    
    if (bPaused)
    {
        // 暂停时显示 SkillListPaused 通知
        ShowNotification(EMANotificationType::SkillListPaused);
    }
    else
    {
        // 恢复时显示 SkillListResumed 通知，2 秒后自动隐藏
        ShowNotification(EMANotificationType::SkillListResumed);
        
        if (World)
        {
            World->GetTimerManager().SetTimer(
                ResumeNotificationTimerHandle,
                [this]()
                {
                    if (MainHUDWidget)
                    {
                        UMANotificationWidget* NotificationWidget = MainHUDWidget->GetNotification();
                        if (NotificationWidget && NotificationWidget->GetCurrentNotificationType() == EMANotificationType::SkillListResumed)
                        {
                            NotificationWidget->HideNotification();
                            UE_LOG(LogMAUIManager, Log, TEXT("OnExecutionPauseStateChanged: Auto-hiding SkillListResumed notification"));
                        }
                    }
                },
                2.0f,
                false
            );
        }
    }
}

//=============================================================================
// EmergencyManager 事件绑定 (Requirements: 4.1, 4.2, 4.3)
//=============================================================================

void UMAUIManager::BindEmergencyManagerEvents()
{
    // 获取 EmergencyManager
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    if (!World)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindEmergencyManagerEvents: World not available"));
        return;
    }
    
    UMAEmergencyManager* EmergencyManager = World->GetSubsystem<UMAEmergencyManager>();
    if (!EmergencyManager)
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("BindEmergencyManagerEvents: EmergencyManager not available"));
        return;
    }
    
    // 绑定紧急事件接收委托 - Requirements: 4.1
    if (!EmergencyManager->OnEmergencyEventReceived.IsAlreadyBound(this, &UMAUIManager::OnEmergencyEventReceived))
    {
        EmergencyManager->OnEmergencyEventReceived.AddDynamic(this, &UMAUIManager::OnEmergencyEventReceived);
        UE_LOG(LogMAUIManager, Log, TEXT("BindEmergencyManagerEvents: Bound OnEmergencyEventReceived"));
    }
    
    UE_LOG(LogMAUIManager, Log, TEXT("BindEmergencyManagerEvents: EmergencyManager events bound"));
}

void UMAUIManager::OnEmergencyEventReceived(const FMAEmergencyEventData& EventData)
{
    // Requirements: 4.1, 4.2, 4.3
    UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyEventReceived: Received emergency event - AgentId=%s, EventType=%s"),
        *EventData.SourceAgentId, *EventData.EventType);
    
    // 显示紧急事件通知 - Requirements: 4.2, 4.3
    // 通知消息: "Unexpected Situation Detected"
    // 按键提示: "Press 'X' to check Unexpected Situation"
    ShowNotification(EMANotificationType::EmergencyEvent);
    
    // 将事件数据加载到 EmergencyModal，以便用户按 X 键时可以显示
    if (EmergencyModal)
    {
        EmergencyModal->LoadEmergencyEvent(EventData);
        UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyEventReceived: Event data loaded into EmergencyModal"));
        
        // 更新相机源 - 从 EmergencyManager 获取 Source Agent 的相机
        UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
        if (World)
        {
            UMAEmergencyManager* EmergencyManager = World->GetSubsystem<UMAEmergencyManager>();
            if (EmergencyManager)
            {
                AMACharacter* SourceAgent = EmergencyManager->GetSourceAgent();
                UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyEventReceived: SourceAgent from EmergencyManager = %s"),
                    SourceAgent ? *SourceAgent->AgentID : TEXT("nullptr"));
                    
                if (SourceAgent)
                {
                    UMACameraSensorComponent* Camera = SourceAgent->GetCameraSensor();
                    UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyEventReceived: Camera = %s"),
                        Camera ? TEXT("Valid") : TEXT("nullptr"));
                        
                    if (Camera)
                    {
                        EmergencyModal->SetCameraSource(Camera);
                        UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyEventReceived: Camera source set from SourceAgent %s"),
                            *SourceAgent->AgentID);
                    }
                    else
                    {
                        UE_LOG(LogMAUIManager, Warning, TEXT("OnEmergencyEventReceived: SourceAgent %s has no camera"),
                            *SourceAgent->AgentID);
                    }
                }
                else
                {
                    UE_LOG(LogMAUIManager, Warning, TEXT("OnEmergencyEventReceived: No SourceAgent available"));
                }
            }
            else
            {
                UE_LOG(LogMAUIManager, Warning, TEXT("OnEmergencyEventReceived: EmergencyManager not found"));
            }
        }
        else
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("OnEmergencyEventReceived: World not available"));
        }
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnEmergencyEventReceived: EmergencyModal is null"));
    }
}

void UMAUIManager::OnEmergencyModalConfirmed(const FMAEmergencyEventData& Data)
{
    // Requirements: 7.1, 7.4, 7.5
    UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyModalConfirmed: Sending response to backend - SelectedOption=%s, UserInputText=%s"),
        *Data.SelectedOption, *Data.UserInputText);
    
    // 获取 CommSubsystem 并发送响应
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    
    if (GameInstance)
    {
        UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>();
        if (CommSubsystem)
        {
            // 发送 button_event 消息 - 与 Reject 按钮保持一致
            // 如果有选中的选项，使用选项作为 button_id，否则使用 "confirm"
            FString ButtonId = Data.SelectedOption.IsEmpty() ? TEXT("confirm") : Data.SelectedOption;
            FString ButtonText = Data.SelectedOption.IsEmpty() ? TEXT("Submit") : Data.SelectedOption;
            CommSubsystem->SendButtonEventMessage(TEXT("EmergencyModal"), ButtonId, ButtonText);
            UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyModalConfirmed: Button event sent - ButtonId=%s, ButtonText=%s"), *ButtonId, *ButtonText);
            
            // 发送紧急事件响应到后端 - Requirements: 7.1, 7.3
            CommSubsystem->GetOutbound()->SendEmergencyResponse(Data);
            UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyModalConfirmed: Emergency response sent to backend"));
        }
        else
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("OnEmergencyModalConfirmed: CommSubsystem not available"));
        }
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnEmergencyModalConfirmed: GameInstance not available"));
    }
    
    // 清除紧急事件通知 - 用户已确认，现在可以清除通知
    if (HUDStateManager)
    {
        HUDStateManager->DismissNotification();
    }
    if (MainHUDWidget && MainHUDWidget->GetNotification())
    {
        MainHUDWidget->GetNotification()->HideNotification();
    }
    
    // 结束 EmergencyManager 中的事件
    if (World)
    {
        UMAEmergencyManager* EmergencyManager = World->GetSubsystem<UMAEmergencyManager>();
        if (EmergencyManager)
        {
            EmergencyManager->EndEvent();
            UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyModalConfirmed: Emergency event ended"));
        }
    }
}

void UMAUIManager::OnEmergencyModalRejected()
{
    // Requirements: 7.1, 7.4
    UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyModalRejected: User rejected emergency event"));
    
    // 获取当前事件数据用于发送拒绝响应
    FMAEmergencyEventData RejectData;
    if (EmergencyModal)
    {
        RejectData = EmergencyModal->GetEmergencyEventData();
        RejectData.SelectedOption = TEXT("Rejected");
        RejectData.UserInputText = TEXT("");
    }
    
    // 获取 CommSubsystem 并发送拒绝响应
    UWorld* World = OwningPC ? OwningPC->GetWorld() : nullptr;
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    
    if (GameInstance)
    {
        UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>();
        if (CommSubsystem)
        {
            // 发送拒绝响应到后端 - Requirements: 7.1, 7.4
            CommSubsystem->GetOutbound()->SendEmergencyResponse(RejectData);
            UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyModalRejected: Rejection response sent to backend"));
        }
        else
        {
            UE_LOG(LogMAUIManager, Warning, TEXT("OnEmergencyModalRejected: CommSubsystem not available"));
        }
    }
    else
    {
        UE_LOG(LogMAUIManager, Warning, TEXT("OnEmergencyModalRejected: GameInstance not available"));
    }
    
    // 清除紧急事件通知 - 用户已拒绝，现在可以清除通知
    if (HUDStateManager)
    {
        HUDStateManager->DismissNotification();
    }
    if (MainHUDWidget && MainHUDWidget->GetNotification())
    {
        MainHUDWidget->GetNotification()->HideNotification();
    }
    
    // 结束 EmergencyManager 中的事件
    if (World)
    {
        UMAEmergencyManager* EmergencyManager = World->GetSubsystem<UMAEmergencyManager>();
        if (EmergencyManager)
        {
            EmergencyManager->EndEvent();
            UE_LOG(LogMAUIManager, Log, TEXT("OnEmergencyModalRejected: Emergency event ended"));
        }
    }
}
