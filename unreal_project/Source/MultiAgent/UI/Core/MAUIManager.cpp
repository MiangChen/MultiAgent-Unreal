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
#include "../Modal/MASkillListModal.h"
#include "../Modal/MAEmergencyModal.h"
#include "../Modal/MABaseModalWidget.h"
#include "../Components/MANotificationWidget.h"
#include "../TaskGraph/MATaskPlannerWidget.h"
#include "../SkillList/MASkillAllocationViewer.h"
#include "../Components/MADirectControlIndicator.h"
#include "../Mode/MAEmergencyWidget.h"
#include "../Mode/MAModifyWidget.h"
#include "../Mode/MAEditWidget.h"
#include "../Mode/MASceneListWidget.h"
#include "../../Core/Manager/MATempDataManager.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
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
        
        // 应用主题
        if (CurrentTheme)
        {
            MainHUDWidget->ApplyTheme(CurrentTheme);
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

    // 绑定 TempDataManager 事件，以便在数据变化时更新预览组件
    BindTempDataManagerEvents();
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

UMABaseModalWidget* UMAUIManager::GetModalByType(EMAModalType ModalType) const
{
    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        return TaskGraphModal;
    case EMAModalType::SkillList:
        return SkillListModal;
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
    
    if (SkillListModal && SkillListModal->IsVisible())
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
        
        // 应用主题
        if (CurrentTheme)
        {
            TaskGraphModal->ApplyTheme(CurrentTheme);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("TaskGraphModal created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create TaskGraphModal"));
    }

    // 创建技能列表模态窗口
    SkillListModal = CreateWidget<UMASkillListModal>(OwningPC, UMASkillListModal::StaticClass());
    if (SkillListModal)
    {
        SkillListModal->AddToViewport(ModalZOrder);
        SkillListModal->SetVisibility(ESlateVisibility::Collapsed);
        SkillListModal->SetModalType(EMAModalType::SkillList);
        
        // 绑定模态按钮回调到 HUDStateManager
        if (HUDStateManager)
        {
            SkillListModal->OnConfirmClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalConfirm);
            SkillListModal->OnRejectClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalReject);
            SkillListModal->OnEditClicked.AddDynamic(HUDStateManager, &UMAHUDStateManager::OnModalEdit);
        }
        
        // 应用主题
        if (CurrentTheme)
        {
            SkillListModal->ApplyTheme(CurrentTheme);
        }
        
        UE_LOG(LogMAUIManager, Log, TEXT("SkillListModal created"));
    }
    else
    {
        UE_LOG(LogMAUIManager, Error, TEXT("Failed to create SkillListModal"));
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
        
        // 应用主题
        if (CurrentTheme)
        {
            EmergencyModal->ApplyTheme(CurrentTheme);
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
        break;

    case EMAHUDState::ReviewModal:
        // 显示只读模态窗口
        if (HUDStateManager)
        {
            ShowModal(HUDStateManager->GetActiveModalType(), false);
        }
        break;

    case EMAHUDState::EditingModal:
        // 编辑模式由 OnModalEditRequested 处理
        // 这里不需要额外操作，因为 OnModalEdit() 会触发 OnModalEditRequested
        // 如果是直接进入编辑模式（如突发事件），则显示对应的编辑界面
        if (HUDStateManager)
        {
            EMAModalType ModalType = HUDStateManager->GetActiveModalType();
            // 对于突发事件，直接显示编辑界面
            if (ModalType == EMAModalType::Emergency)
            {
                ShowWidget(EMAWidgetType::Emergency, true);
            }
        }
        break;
    }
}

void UMAUIManager::OnNotificationReceived(EMANotificationType Type)
{
    UE_LOG(LogMAUIManager, Log, TEXT("Notification received: %s"),
        *UEnum::GetValueAsString(Type));

    // 在 MainHUDWidget 的通知组件中显示通知
    if (MainHUDWidget)
    {
        UMANotificationWidget* NotificationWidget = MainHUDWidget->GetNotification();
        if (NotificationWidget)
        {
            NotificationWidget->ShowNotification(Type);
        }
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

    case EMAModalType::SkillList:
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

    if (SkillListModal && SkillListModal->IsVisible())
    {
        SkillListModal->PlayHideAnimation();
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

    // Mock 数据文件路径 (在项目根目录的 datasets 文件夹中)
    // FPaths::ProjectDir() 返回 unreal_project/，所以需要向上一级
    FString DatasetsDir = FPaths::ProjectDir() / TEXT("../datasets");

    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        if (TaskGraphModal)
        {
            // 优先从 TempDataManager 加载
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
            
            // 回退到 mock 数据文件
            FString MockFilePath = DatasetsDir / TEXT("response_example.json");
            if (FPaths::FileExists(MockFilePath))
            {
                FString JsonContent;
                if (FFileHelper::LoadFileToString(JsonContent, *MockFilePath))
                {
                    FMATaskGraphData Data;
                    FString ErrorMessage;
                    if (FMATaskGraphData::FromResponseJson(JsonContent, Data, ErrorMessage))
                    {
                        TaskGraphModal->LoadTaskGraph(Data);
                        UE_LOG(LogMAUIManager, Log, TEXT("TaskGraphModal: Loaded mock data (%d nodes, %d edges)"),
                            Data.Nodes.Num(), Data.Edges.Num());
                        return;
                    }
                    else
                    {
                        UE_LOG(LogMAUIManager, Warning, TEXT("TaskGraphModal: Failed to parse mock data: %s"), *ErrorMessage);
                    }
                }
            }
            else
            {
                UE_LOG(LogMAUIManager, Warning, TEXT("TaskGraphModal: Mock file not found: %s"), *MockFilePath);
            }
            
            // 如果都失败，创建空数据
            UE_LOG(LogMAUIManager, Warning, TEXT("TaskGraphModal: No data available, showing empty modal"));
        }
        break;

    case EMAModalType::SkillList:
        if (SkillListModal)
        {
            // 只从 TempDataManager 加载数据，不使用 mock 数据
            if (TempDataMgr && TempDataMgr->SkillListFileExists())
            {
                FMASkillAllocationData Data;
                if (TempDataMgr->LoadSkillList(Data))
                {
                    SkillListModal->LoadSkillAllocation(Data);
                    UE_LOG(LogMAUIManager, Log, TEXT("SkillListModal: Loaded data from temp file (%d time steps)"),
                        Data.Data.Num());
                    return;
                }
            }
            
            // 如果没有数据，清空 Modal 并显示空状态（不加载 mock 数据）
            FMASkillAllocationData EmptyData;
            SkillListModal->LoadSkillAllocation(EmptyData);
            UE_LOG(LogMAUIManager, Log, TEXT("SkillListModal: No data available, showing empty modal (waiting for backend data)"));
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

EMAModalType UMAUIManager::GetModalTypeForNotification(EMANotificationType NotificationType) const
{
    switch (NotificationType)
    {
    case EMANotificationType::TaskGraphUpdate:
        return EMAModalType::TaskGraph;

    case EMANotificationType::SkillListUpdate:
        return EMAModalType::SkillList;

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
    
    // 绑定技能列表数据变化事件
    if (!TempDataMgr->OnSkillListChanged.IsAlreadyBound(this, &UMAUIManager::OnTempSkillListChanged))
    {
        TempDataMgr->OnSkillListChanged.AddDynamic(this, &UMAUIManager::OnTempSkillListChanged);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Bound OnSkillListChanged event"));
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
    
    FMASkillAllocationData SkillListData;
    if (TempDataMgr->LoadSkillList(SkillListData) && SkillListData.Data.Num() > 0)
    {
        OnTempSkillListChanged(SkillListData);
        UE_LOG(LogMAUIManager, Log, TEXT("BindTempDataManagerEvents: Loaded existing skill list data (%d time steps)"), 
            SkillListData.Data.Num());
    }
}

void UMAUIManager::OnTempTaskGraphChanged(const FMATaskGraphData& Data)
{
    UE_LOG(LogMAUIManager, Log, TEXT("OnTempTaskGraphChanged: Received task graph update (%d nodes, %d edges)"),
        Data.Nodes.Num(), Data.Edges.Num());
    
    // 更新 MainHUDWidget 的预览组件
    if (MainHUDWidget)
    {
        MainHUDWidget->UpdateTaskGraphPreview(Data);
    }
}

void UMAUIManager::OnTempSkillListChanged(const FMASkillAllocationData& Data)
{
    UE_LOG(LogMAUIManager, Log, TEXT("OnTempSkillListChanged: Received skill list update (%d time steps)"),
        Data.Data.Num());
    
    // 更新 MainHUDWidget 的预览组件
    if (MainHUDWidget)
    {
        MainHUDWidget->UpdateSkillListPreview(Data);
    }
}
