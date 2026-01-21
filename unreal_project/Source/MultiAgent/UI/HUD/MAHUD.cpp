// MAHUD.cpp
// HUD 管理器实现
// 继承自 MASelectionHUD 以保留框选绘制功能
// Widget 管理职责已委托给 UMAUIManager

#include "MAHUD.h"
#include "../Core/MAUIManager.h"
#include "../Core/MAUITheme.h"
#include "MAHUDWidget.h"
#include "../Core/MAHUDStateManager.h"
#include "MAMainHUDWidget.h"
#include "../Legacy/MASimpleMainWidget.h"
#include "../Modal/MATaskGraphModal.h"
#include "../Modal/MASkillAllocationModal.h"
#include "../Modal/MAEmergencyModal.h"
#include "../TaskGraph/MATaskPlannerWidget.h"
#include "../SkillAllocation/MASkillAllocationViewer.h"
#include "../Components/MADirectControlIndicator.h"
#include "../Components/MARightSidebarWidget.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "../Mode/MAEmergencyWidget.h"
#include "../Mode/MAModifyWidget.h"
#include "../Mode/MAEditWidget.h"
#include "../Mode/MASceneListWidget.h"
#include "../../Input/MAPlayerController.h"
#include "../../Core/Comm/MACommSubsystem.h"
#include "../../Core/Comm/MACommTypes.h"
#include "../../Core/Types/MASimTypes.h"
#include "../../Core/MASubsystem.h"
#include "../../Core/Manager/MAEmergencyManager.h"
#include "../../Core/Manager/MAEditModeManager.h"
#include "../../Core/Manager/MASceneGraphManager.h"
#include "../../Agent/Character/MACharacter.h"
#include "../../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "../../Environment/MAPointOfInterest.h"
#include "../../Environment/MAZoneActor.h"
#include "../../Environment/MAGoalActor.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"
#include "DrawDebugHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUD, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

AMAHUD::AMAHUD()
{
    // 默认值
    bMainUIVisible = false;
    UIManager = nullptr;
}

//=============================================================================
// AHUD 重写
//=============================================================================

void AMAHUD::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogMAHUD, Log, TEXT("MAHUD BeginPlay"));

    // 创建并初始化 UIManager
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        UIManager = NewObject<UMAUIManager>(this);
        if (UIManager)
        {
            // 传递 SemanticMapWidgetClass 给 UIManager
            UIManager->SemanticMapWidgetClass = SemanticMapWidgetClass;
            
            UIManager->Initialize(PC);
            
            // 加载并应用主题 (Requirements: 1.4)
            UIManager->LoadTheme(UIThemeAsset);
            UE_LOG(LogMAHUD, Log, TEXT("UI Theme loaded"));
            
            UIManager->CreateAllWidgets();
            UE_LOG(LogMAHUD, Log, TEXT("UIManager created and initialized"));
        }
        else
        {
            UE_LOG(LogMAHUD, Error, TEXT("Failed to create UIManager"));
        }
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("BeginPlay: No owning PlayerController"));
    }

    // 绑定 Widget 委托 (在 UIManager 创建 Widget 后)
    BindWidgetDelegates();

    // 绑定 PlayerController 事件
    BindControllerEvents();

    // 绑定 EmergencyManager 事件
    BindEmergencyManagerEvents();

    // 绑定 EditModeManager 事件
    BindEditModeManagerEvents();

    // 绑定 EditWidget 委托
    BindEditWidgetDelegates();
    
    // 绑定后端事件到 HUD 状态管理器 (Requirements: 4.1, 4.2, 4.3, 12.1)
    BindBackendEvents();
}

//=============================================================================
// 公共接口
//=============================================================================

void AMAHUD::ToggleMainUI()
{
    if (bMainUIVisible)
    {
        HideMainUI();
    }
    else
    {
        ShowMainUI();
    }
}

void AMAHUD::ShowMainUI()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowMainUI: UIManager is null"));
        return;
    }

    if (bMainUIVisible)
    {
        // 已经显示，只需重新聚焦
        UIManager->SetWidgetFocus(EMAWidgetType::TaskPlanner);
        return;
    }

    // 通过 UIManager 显示 TaskPlannerWidget
    if (UIManager->ShowWidget(EMAWidgetType::TaskPlanner, true))
    {
        bMainUIVisible = true;
        UE_LOG(LogMAHUD, Log, TEXT("MainUI shown via UIManager"));
    }
}

void AMAHUD::HideMainUI()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideMainUI: UIManager is null"));
        return;
    }

    if (!bMainUIVisible)
    {
        return;
    }

    // 通过 UIManager 隐藏 TaskPlannerWidget
    if (UIManager->HideWidget(EMAWidgetType::TaskPlanner))
    {
        bMainUIVisible = false;
        UE_LOG(LogMAHUD, Log, TEXT("MainUI hidden via UIManager"));
    }
}

void AMAHUD::ToggleSemanticMap()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ToggleSemanticMap: UIManager is null"));
        return;
    }

    UIManager->ToggleWidget(EMAWidgetType::SemanticMap);
    UE_LOG(LogMAHUD, Log, TEXT("SemanticMap toggled via UIManager"));
}

void AMAHUD::ShowEmergencyWidget()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowEmergencyWidget: UIManager is null"));
        return;
    }

    UMAEmergencyWidget* EmergencyWidget = UIManager->GetEmergencyWidget();
    if (!EmergencyWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowEmergencyWidget: EmergencyWidget is null"));
        return;
    }

    if (IsEmergencyWidgetVisible())
    {
        // 已经显示，只需重新聚焦
        EmergencyWidget->FocusInputBox();
        return;
    }

    // 通过 UIManager 显示
    UIManager->ShowWidget(EMAWidgetType::Emergency, true);

    // 更新相机源
    FMASubsystem Subs = MA_SUBS;
    if (Subs.EmergencyManager && Subs.EmergencyManager->IsEventActive())
    {
        AMACharacter* SourceAgent = Subs.EmergencyManager->GetSourceAgent();
        if (SourceAgent)
        {
            UMACameraSensorComponent* Camera = SourceAgent->GetCameraSensor();
            UpdateEmergencyCameraSource(Camera);
        }
        else
        {
            // 没有 Source Agent，显示黑屏
            UpdateEmergencyCameraSource(nullptr);
        }
    }

    UE_LOG(LogMAHUD, Log, TEXT("EmergencyWidget shown via UIManager"));
}

void AMAHUD::HideEmergencyWidget()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideEmergencyWidget: UIManager is null"));
        return;
    }

    if (!IsEmergencyWidgetVisible())
    {
        return;
    }

    // 通过 UIManager 隐藏
    UIManager->HideWidget(EMAWidgetType::Emergency);

    UE_LOG(LogMAHUD, Log, TEXT("EmergencyWidget hidden via UIManager"));
}

void AMAHUD::ToggleEmergencyWidget()
{
    if (IsEmergencyWidgetVisible())
    {
        HideEmergencyWidget();
    }
    else
    {
        ShowEmergencyWidget();
    }
}

bool AMAHUD::IsEmergencyWidgetVisible() const
{
    if (!UIManager)
    {
        return false;
    }
    
    return UIManager->IsWidgetVisible(EMAWidgetType::Emergency);
}

void AMAHUD::UpdateEmergencyCameraSource(UMACameraSensorComponent* Camera)
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("UpdateEmergencyCameraSource: UIManager is null"));
        return;
    }

    UMAEmergencyWidget* EmergencyWidget = UIManager->GetEmergencyWidget();
    if (!EmergencyWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("UpdateEmergencyCameraSource: EmergencyWidget is null"));
        return;
    }

    if (Camera)
    {
        EmergencyWidget->SetCameraSource(Camera);
        UE_LOG(LogMAHUD, Log, TEXT("EmergencyWidget camera source updated"));
    }
    else
    {
        EmergencyWidget->ClearCameraSource();
        UE_LOG(LogMAHUD, Log, TEXT("EmergencyWidget camera source cleared"));
    }
}

//=============================================================================
// Emergency Indicator
//=============================================================================

void AMAHUD::ShowEmergencyIndicator()
{
    if (bShowEmergencyIndicator)
    {
        return;  // Already shown
    }

    bShowEmergencyIndicator = true;

    // 委托到 HUDWidget
    if (UIManager)
    {
        UMAHUDWidget* HUDWidget = UIManager->GetHUDWidget();
        if (HUDWidget)
        {
            HUDWidget->ShowEmergencyIndicator();
        }
    }

    UE_LOG(LogMAHUD, Log, TEXT("Emergency indicator shown"));
}

void AMAHUD::HideEmergencyIndicator()
{
    if (!bShowEmergencyIndicator)
    {
        return;  // Already hidden
    }

    bShowEmergencyIndicator = false;

    // 委托到 HUDWidget
    if (UIManager)
    {
        UMAHUDWidget* HUDWidget = UIManager->GetHUDWidget();
        if (HUDWidget)
        {
            HUDWidget->HideEmergencyIndicator();
        }
    }

    UE_LOG(LogMAHUD, Log, TEXT("Emergency indicator hidden"));
}

void AMAHUD::DrawHUD()
{
    Super::DrawHUD();

    // Emergency 指示器已迁移到 HUDWidget，不再使用 Canvas 绘制

    // Draw scene labels (Modify mode) - 保留，使用 DrawDebugString (世界空间)
    DrawSceneLabels();

    // Edit 模式指示器 - 数据收集并更新 HUDWidget
    DrawEditModeIndicator();
}

void AMAHUD::ShowDirectControlIndicator(AMACharacter* Agent)
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowDirectControlIndicator: UIManager is null"));
        return;
    }

    UMADirectControlIndicator* DirectControlIndicator = UIManager->GetDirectControlIndicator();
    if (!DirectControlIndicator)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowDirectControlIndicator: DirectControlIndicator is null"));
        return;
    }

    if (!Agent)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowDirectControlIndicator: Agent is null"));
        return;
    }

    // 设置 Agent 名称
    DirectControlIndicator->SetAgentName(Agent->AgentName);
    
    // 通过 UIManager 显示指示器
    UIManager->ShowWidget(EMAWidgetType::DirectControl, false);
    
    UE_LOG(LogMAHUD, Log, TEXT("DirectControlIndicator shown for Agent: %s"), *Agent->AgentName);
}

void AMAHUD::HideDirectControlIndicator()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideDirectControlIndicator: UIManager is null"));
        return;
    }

    UIManager->HideWidget(EMAWidgetType::DirectControl);
    
    UE_LOG(LogMAHUD, Log, TEXT("DirectControlIndicator hidden via UIManager"));
}

bool AMAHUD::IsDirectControlIndicatorVisible() const
{
    if (!UIManager)
    {
        return false;
    }
    
    return UIManager->IsWidgetVisible(EMAWidgetType::DirectControl);
}

//=============================================================================
// 技能分配查看器控制
//=============================================================================

void AMAHUD::ToggleSkillAllocationViewer()
{
    if (IsSkillAllocationViewerVisible())
    {
        HideSkillAllocationViewer();
    }
    else
    {
        ShowSkillAllocationViewer();
    }
}

void AMAHUD::ShowSkillAllocationViewer()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowSkillAllocationViewer: UIManager is null"));
        return;
    }

    if (IsSkillAllocationViewerVisible())
    {
        // 已经显示
        return;
    }

    // 通过 UIManager 显示
    UIManager->ShowWidget(EMAWidgetType::SkillAllocation, true);

    UE_LOG(LogMAHUD, Log, TEXT("SkillAllocationViewer shown via UIManager"));
}

void AMAHUD::HideSkillAllocationViewer()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideSkillAllocationViewer: UIManager is null"));
        return;
    }

    if (!IsSkillAllocationViewerVisible())
    {
        return;
    }

    // 通过 UIManager 隐藏
    UIManager->HideWidget(EMAWidgetType::SkillAllocation);

    UE_LOG(LogMAHUD, Log, TEXT("SkillAllocationViewer hidden via UIManager"));
}

bool AMAHUD::IsSkillAllocationViewerVisible() const
{
    if (!UIManager)
    {
        return false;
    }
    
    return UIManager->IsWidgetVisible(EMAWidgetType::SkillAllocation);
}

//=============================================================================
// 事件回调
//=============================================================================

void AMAHUD::OnMainUIToggled(bool bVisible)
{
    UE_LOG(LogMAHUD, Verbose, TEXT("OnMainUIToggled: %s"), bVisible ? TEXT("Show") : TEXT("Hide"));

    if (bVisible)
    {
        ShowMainUI();
    }
    else
    {
        HideMainUI();
    }
}

void AMAHUD::OnRefocusMainUI()
{
    UE_LOG(LogMAHUD, Verbose, TEXT("OnRefocusMainUI"));

    if (bMainUIVisible && UIManager)
    {
        UIManager->SetWidgetFocus(EMAWidgetType::TaskPlanner);
    }
}

void AMAHUD::OnEmergencyStateChanged(bool bIsActive)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnEmergencyStateChanged: %s"), bIsActive ? TEXT("Active") : TEXT("Inactive"));

    if (bIsActive)
    {
        ShowEmergencyIndicator();
    }
    else
    {
        HideEmergencyIndicator();
        
        // 如果 Widget 可见且事件结束，清除相机源（显示黑屏）
        if (IsEmergencyWidgetVisible())
        {
            UpdateEmergencyCameraSource(nullptr);
        }
    }
}

//=============================================================================
// 内部方法
//=============================================================================

void AMAHUD::BindWidgetDelegates()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindWidgetDelegates: UIManager is null"));
        return;
    }

    // 绑定 TaskPlannerWidget 委托
    UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget();
    if (TaskPlannerWidget)
    {
        if (!TaskPlannerWidget->OnCommandSubmitted.IsAlreadyBound(this, &AMAHUD::OnSimpleCommandSubmitted))
        {
            TaskPlannerWidget->OnCommandSubmitted.AddDynamic(this, &AMAHUD::OnSimpleCommandSubmitted);
        }
        UE_LOG(LogMAHUD, Log, TEXT("BindWidgetDelegates: Bound TaskPlannerWidget delegates"));
    }

    // 绑定 SimpleMainWidget 委托 (Legacy)
    UMASimpleMainWidget* SimpleMainWidget = UIManager->GetSimpleMainWidget();
    if (SimpleMainWidget)
    {
        if (!SimpleMainWidget->OnCommandSubmitted.IsAlreadyBound(this, &AMAHUD::OnSimpleCommandSubmitted))
        {
            SimpleMainWidget->OnCommandSubmitted.AddDynamic(this, &AMAHUD::OnSimpleCommandSubmitted);
        }
        UE_LOG(LogMAHUD, Log, TEXT("BindWidgetDelegates: Bound SimpleMainWidget delegates"));
    }

    // 绑定 RightSidebarWidget 委托 (Command Input in sidebar)
    UMAMainHUDWidget* MainHUDWidget = UIManager->GetMainHUDWidget();
    if (MainHUDWidget)
    {
        UMARightSidebarWidget* RightSidebar = MainHUDWidget->GetRightSidebar();
        if (RightSidebar)
        {
            if (!RightSidebar->OnCommandSubmitted.IsAlreadyBound(this, &AMAHUD::OnSimpleCommandSubmitted))
            {
                RightSidebar->OnCommandSubmitted.AddDynamic(this, &AMAHUD::OnSimpleCommandSubmitted);
            }
            UE_LOG(LogMAHUD, Log, TEXT("BindWidgetDelegates: Bound RightSidebarWidget delegates"));
        }
    }

    // 绑定 ModifyWidget 委托
    UMAModifyWidget* ModifyWidget = UIManager->GetModifyWidget();
    if (ModifyWidget)
    {
        if (!ModifyWidget->OnModifyConfirmed.IsAlreadyBound(this, &AMAHUD::OnModifyConfirmed))
        {
            ModifyWidget->OnModifyConfirmed.AddDynamic(this, &AMAHUD::OnModifyConfirmed);
        }
        if (!ModifyWidget->OnMultiSelectModifyConfirmed.IsAlreadyBound(this, &AMAHUD::OnMultiSelectModifyConfirmed))
        {
            ModifyWidget->OnMultiSelectModifyConfirmed.AddDynamic(this, &AMAHUD::OnMultiSelectModifyConfirmed);
        }
        UE_LOG(LogMAHUD, Log, TEXT("BindWidgetDelegates: Bound ModifyWidget delegates"));
    }

    // 绑定 SceneListWidget 委托
    UMASceneListWidget* SceneListWidget = UIManager->GetSceneListWidget();
    if (SceneListWidget)
    {
        // 设置 EditModeManager 引用
        UWorld* World = GetWorld();
        if (World)
        {
            UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
            if (EditModeManager)
            {
                SceneListWidget->SetEditModeManager(EditModeManager);
            }
        }

        if (!SceneListWidget->OnGoalItemClicked.IsAlreadyBound(this, &AMAHUD::OnSceneListGoalClicked))
        {
            SceneListWidget->OnGoalItemClicked.AddDynamic(this, &AMAHUD::OnSceneListGoalClicked);
        }
        if (!SceneListWidget->OnZoneItemClicked.IsAlreadyBound(this, &AMAHUD::OnSceneListZoneClicked))
        {
            SceneListWidget->OnZoneItemClicked.AddDynamic(this, &AMAHUD::OnSceneListZoneClicked);
        }
        UE_LOG(LogMAHUD, Log, TEXT("BindWidgetDelegates: Bound SceneListWidget delegates"));
    }

    UE_LOG(LogMAHUD, Log, TEXT("BindWidgetDelegates: All widget delegates bound"));
}

void AMAHUD::BindControllerEvents()
{
    AMAPlayerController* MAPC = GetMAPlayerController();
    if (!MAPC)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindControllerEvents: MAPlayerController not found"));
        return;
    }

    // 绑定 Modify 模式 Actor 选中委托 (单选模式)
    MAPC->OnModifyActorSelected.AddDynamic(this, &AMAHUD::OnModifyActorSelected);
    UE_LOG(LogMAHUD, Log, TEXT("BindControllerEvents: Bound OnModifyActorSelected delegate"));

    // 绑定 Modify 模式 Actor 选中委托 (多选模式)
    MAPC->OnModifyActorsSelected.AddDynamic(this, &AMAHUD::OnModifyActorsSelected);
    UE_LOG(LogMAHUD, Log, TEXT("BindControllerEvents: Bound OnModifyActorsSelected delegate"));
    UE_LOG(LogMAHUD, Log, TEXT("BindControllerEvents: Ready to bind when MAPlayerController delegates are available"));
}

void AMAHUD::BindEmergencyManagerEvents()
{
    FMASubsystem Subs = MA_SUBS;
    if (!Subs.EmergencyManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindEmergencyManagerEvents: EmergencyManager not found"));
        return;
    }

    // 绑定事件状态变化委托
    Subs.EmergencyManager->OnEmergencyStateChanged.AddDynamic(this, &AMAHUD::OnEmergencyStateChanged);
    
    UE_LOG(LogMAHUD, Log, TEXT("BindEmergencyManagerEvents: Bound to EmergencyManager"));

    // 同步当前状态 (如果 HUD 创建时事件已经激活)
    if (Subs.EmergencyManager->IsEventActive())
    {
        ShowEmergencyIndicator();
    }
}

AMAPlayerController* AMAHUD::GetMAPlayerController() const
{
    return Cast<AMAPlayerController>(GetOwningPlayerController());
}

void AMAHUD::OnSimpleCommandSubmitted(const FString& Command)
{
    UE_LOG(LogMAHUD, Log, TEXT("Command submitted: %s"), *Command);
    
    // 获取 CommSubsystem 并发送指令
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>())
        {
            // 绑定响应回调 (只绑定一次)
            if (!CommSubsystem->OnPlannerResponse.IsAlreadyBound(this, &AMAHUD::OnPlannerResponse))
            {
                CommSubsystem->OnPlannerResponse.AddDynamic(this, &AMAHUD::OnPlannerResponse);
            }
            
            // 发送指令
            CommSubsystem->SendNaturalLanguageCommand(Command);
        }
        else
        {
            UE_LOG(LogMAHUD, Warning, TEXT("CommSubsystem not found"));
            UMASimpleMainWidget* SimpleMainWidget = UIManager ? UIManager->GetSimpleMainWidget() : nullptr;
            if (SimpleMainWidget)
            {
                SimpleMainWidget->SetResultText(TEXT("Error: Communication subsystem not found"));
            }
        }
    }
}

void AMAHUD::OnPlannerResponse(const FMAPlannerResponse& Response)
{
    UE_LOG(LogMAHUD, Log, TEXT("Planner response received: %s"), *Response.Message);
    
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("OnPlannerResponse: UIManager is null"));
        return;
    }

    // 显示响应到 TaskPlannerWidget 或 SimpleMainWidget
    UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget();
    UMASimpleMainWidget* SimpleMainWidget = UIManager->GetSimpleMainWidget();
    
    if (TaskPlannerWidget)
    {
        // 追加状态日志
        TaskPlannerWidget->AppendStatusLog(FString::Printf(TEXT("[%s] %s"), 
            Response.bSuccess ? TEXT("Success") : TEXT("Failed"),
            *Response.Message));
        
        // 如果有规划数据，尝试加载
        if (!Response.PlanText.IsEmpty())
        {
            TaskPlannerWidget->LoadTaskGraphFromJson(Response.PlanText);
        }
    }
    else if (SimpleMainWidget)
    {
        FString DisplayText = FString::Printf(TEXT("[%s]\n%s"), 
            Response.bSuccess ? TEXT("Success") : TEXT("Failed"),
            *Response.Message);
        
        if (!Response.PlanText.IsEmpty())
        {
            DisplayText += FString::Printf(TEXT("\n\nPlan:\n%s"), *Response.PlanText);
        }
        
        SimpleMainWidget->SetResultText(DisplayText);
    }
}

void AMAHUD::LoadTaskGraph(const FMATaskGraphData& Data)
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("LoadTaskGraph: UIManager is null"));
        return;
    }

    UMATaskPlannerWidget* TaskPlannerWidget = UIManager->GetTaskPlannerWidget();
    if (TaskPlannerWidget)
    {
        TaskPlannerWidget->LoadTaskGraph(Data);
        UE_LOG(LogMAHUD, Log, TEXT("Task graph loaded to TaskPlannerWidget"));
    }
    else
    {
        UE_LOG(LogMAHUD, Warning, TEXT("LoadTaskGraph: TaskPlannerWidget is null"));
    }
}

//=============================================================================
// Widget Getter 方法 (委托到 UIManager)
//=============================================================================

UMASimpleMainWidget* AMAHUD::GetSimpleMainWidget() const
{
    return UIManager ? UIManager->GetSimpleMainWidget() : nullptr;
}

UMATaskPlannerWidget* AMAHUD::GetTaskPlannerWidget() const
{
    return UIManager ? UIManager->GetTaskPlannerWidget() : nullptr;
}

UMAHUDStateManager* AMAHUD::GetHUDStateManager() const
{
    return UIManager ? UIManager->GetHUDStateManager() : nullptr;
}

UMAMainHUDWidget* AMAHUD::GetMainHUDWidget() const
{
    return UIManager ? UIManager->GetMainHUDWidget() : nullptr;
}

//=============================================================================
// Modify 模式 UI 控制
//=============================================================================

void AMAHUD::ShowModifyWidget()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowModifyWidget: UIManager is null"));
        return;
    }

    if (IsModifyWidgetVisible())
    {
        // 已经显示
        return;
    }

    // 通过 UIManager 显示
    UIManager->ShowWidget(EMAWidgetType::Modify, true);
    StartSceneLabelVisualization();

    UE_LOG(LogMAHUD, Log, TEXT("ModifyWidget shown via UIManager"));
}

void AMAHUD::HideModifyWidget()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideModifyWidget: UIManager is null"));
        return;
    }

    if (!IsModifyWidgetVisible())
    {
        return;
    }
    StopSceneLabelVisualization();

    // 通过 UIManager 隐藏
    UIManager->HideWidget(EMAWidgetType::Modify);

    // 清除选中状态
    UMAModifyWidget* ModifyWidget = UIManager->GetModifyWidget();
    if (ModifyWidget)
    {
        ModifyWidget->ClearSelection();
    }

    UE_LOG(LogMAHUD, Log, TEXT("ModifyWidget hidden via UIManager"));
}

bool AMAHUD::IsModifyWidgetVisible() const
{
    if (!UIManager)
    {
        return false;
    }

    return UIManager->IsWidgetVisible(EMAWidgetType::Modify);
}

//=============================================================================
// Edit 模式 UI 控制
//=============================================================================

void AMAHUD::ShowEditWidget()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowEditWidget: UIManager is null"));
        return;
    }

    if (IsEditWidgetVisible())
    {
        // 已经显示
        return;
    }

    // 通过 UIManager 显示
    UIManager->ShowWidget(EMAWidgetType::Edit, true);

    UE_LOG(LogMAHUD, Log, TEXT("EditWidget shown via UIManager"));
}

void AMAHUD::HideEditWidget()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideEditWidget: UIManager is null"));
        return;
    }

    if (!IsEditWidgetVisible())
    {
        return;
    }

    // 通过 UIManager 隐藏
    UIManager->HideWidget(EMAWidgetType::Edit);

    // 清除选中状态
    UMAEditWidget* EditWidget = UIManager->GetEditWidget();
    if (EditWidget)
    {
        EditWidget->ClearSelection();
    }

    UE_LOG(LogMAHUD, Log, TEXT("EditWidget hidden via UIManager"));
}

bool AMAHUD::IsEditWidgetVisible() const
{
    if (!UIManager)
    {
        return false;
    }

    return UIManager->IsWidgetVisible(EMAWidgetType::Edit);
}

bool AMAHUD::IsMouseOverEditWidget() const
{
    // 如果 EditWidget 不可见，鼠标肯定不在上面
    if (!IsEditWidgetVisible())
    {
        return false;
    }

    // 获取 PlayerController
    APlayerController* PC = GetOwningPlayerController();
    if (!PC)
    {
        return false;
    }

    // 使用 GetMousePosition 获取相对于视口的鼠标位置
    // 这比使用 GetCursorPos + 窗口位置转换更可靠
    float MouseX, MouseY;
    if (!PC->GetMousePosition(MouseX, MouseY))
    {
        return false;
    }

    // 获取视口大小
    int32 ViewportSizeX, ViewportSizeY;
    PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

    // 检查 EditWidget 区域 (右侧面板)
    // EditWidget 布局: 锚点 (1.0, 0.0), 位置 (-20, 60), 大小 (380, 600)
    // 实际屏幕区域: X 从 (ViewportSizeX - 20 - 380) 到 (ViewportSizeX - 20), Y 从 60 到 660
    UMAEditWidget* EditWidget = UIManager ? UIManager->GetEditWidget() : nullptr;
    if (EditWidget)
    {
        float EditWidgetLeft = ViewportSizeX - 20.0f - 380.0f;
        float EditWidgetRight = ViewportSizeX - 20.0f;
        float EditWidgetTop = 60.0f;
        float EditWidgetBottom = 60.0f + 600.0f;

        if (MouseX >= EditWidgetLeft && MouseX <= EditWidgetRight &&
            MouseY >= EditWidgetTop && MouseY <= EditWidgetBottom)
        {
            return true;
        }
    }

    return false;
}

void AMAHUD::OnModifyConfirmed(AActor* Actor, const FString& LabelText)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnModifyConfirmed: Actor=%s, LabelText=%s"),
        Actor ? *Actor->GetName() : TEXT("null"), *LabelText);
    if (!Actor)
    {
        ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GI)
    {
        ShowNotification(TEXT("Save failed: Unable to get GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("Save failed: SceneGraphManager not found"), true);
        return;
    }

    // 获取 ModifyWidget 通过 UIManager
    UMAModifyWidget* ModifyWidget = UIManager ? UIManager->GetModifyWidget() : nullptr;

    FString Id, Type, ErrorMessage;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        ShowNotification(ErrorMessage, true);
        // ModifyWidget 不清空，由 ModifyWidget 自己处理
        // 重新设置选中的 Actor 以保留状态
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActor(Actor);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }
    FVector WorldLocation = Actor->GetActorLocation();
    UE_LOG(LogMAHUD, Log, TEXT("OnModifyConfirmed: Actor location = (%f, %f, %f)"),
        WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
    FString GeneratedLabel;
    if (!SceneGraphManager->AddSceneNode(Id, Type, WorldLocation, Actor, GeneratedLabel, ErrorMessage))
    {
        // 检查是否是 ID 重复的警告
        if (ErrorMessage.Contains(TEXT("ID already exists")))
        {
            ShowNotification(ErrorMessage, false, true);  // 警告样式
            if (ModifyWidget)
            {
                ModifyWidget->ClearSelection();
            }
            ClearHighlightedActor();
        }
        else
        {
            ShowNotification(ErrorMessage, true);  // 错误样式
            if (ModifyWidget)
            {
                ModifyWidget->SetSelectedActor(Actor);
                ModifyWidget->SetLabelText(LabelText);
            }
        }
        return;
    }
    FString SuccessMessage = FString::Printf(TEXT("Label saved: %s"), *GeneratedLabel);
    ShowNotification(SuccessMessage, false);
    // 重新加载场景图数据以显示新添加的节点
    LoadSceneGraphForVisualization();
    if (ModifyWidget)
    {
        ModifyWidget->ClearSelection();
    }

    // 清除高亮的 Actor
    ClearHighlightedActor();

    UE_LOG(LogMAHUD, Log, TEXT("OnModifyConfirmed: Successfully saved node with label=%s"), *GeneratedLabel);
}

void AMAHUD::OnModifyActorSelected(AActor* SelectedActor)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnModifyActorSelected: Actor=%s"),
        SelectedActor ? *SelectedActor->GetName() : TEXT("null"));

    UMAModifyWidget* ModifyWidget = UIManager ? UIManager->GetModifyWidget() : nullptr;
    if (ModifyWidget)
    {
        ModifyWidget->SetSelectedActor(SelectedActor);
    }
}

void AMAHUD::OnModifyActorsSelected(const TArray<AActor*>& SelectedActors)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnModifyActorsSelected: %d actors"), SelectedActors.Num());

    UMAModifyWidget* ModifyWidget = UIManager ? UIManager->GetModifyWidget() : nullptr;
    if (ModifyWidget)
    {
        ModifyWidget->SetSelectedActors(SelectedActors);
    }
}

void AMAHUD::OnMultiSelectModifyConfirmed(const TArray<AActor*>& Actors, const FString& LabelText, const FString& GeneratedJson)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnMultiSelectModifyConfirmed: %d actors, LabelText=%s"),
        Actors.Num(), *LabelText);
    if (Actors.Num() == 0)
    {
        ShowNotification(TEXT("Please select at least one Actor first"), true);
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GI)
    {
        ShowNotification(TEXT("Save failed: Unable to get GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("Save failed: SceneGraphManager not found"), true);
        return;
    }

    // 获取 ModifyWidget 通过 UIManager
    UMAModifyWidget* ModifyWidget = UIManager ? UIManager->GetModifyWidget() : nullptr;

    // 解析输入以获取 ID
    FString Id, Type, ErrorMessage;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        // 解析失败 - 显示错误
        ShowNotification(ErrorMessage, true);

        // 保留输入状态
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActors(Actors);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    // 检查 ID 是否已存在
    if (SceneGraphManager->IsIdExists(Id))
    {
        ShowNotification(FString::Printf(TEXT("ID '%s' already exists, please use a different ID"), *Id), false, true);

        // 保留输入状态
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActors(Actors);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    UE_LOG(LogMAHUD, Log, TEXT("OnMultiSelectModifyConfirmed: Saving JSON:\n%s"), *GeneratedJson);

    // 调用统一的 AddNode API 添加节点到 Working Copy
    if (!SceneGraphManager->AddNode(GeneratedJson, ErrorMessage))
    {
        // 添加失败
        ShowNotification(ErrorMessage, true);

        // 保留输入状态
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActors(Actors);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    // 保存到源文件 (仅 Modify 模式会实际保存)
    SceneGraphManager->SaveToSource();

    // 显示成功消息
    FString SuccessMessage = FString::Printf(TEXT("Multi-select annotation saved: %d Actors"), Actors.Num());
    ShowNotification(SuccessMessage, false);

    // 刷新可视化
    LoadSceneGraphForVisualization();

    // 清空选择
    if (ModifyWidget)
    {
        ModifyWidget->ClearSelection();
    }

    // 清除高亮
    ClearHighlightedActor();

    UE_LOG(LogMAHUD, Log, TEXT("OnMultiSelectModifyConfirmed: Successfully processed multi-select annotation"));
}

//=============================================================================
// 通知系统
//=============================================================================

void AMAHUD::ShowNotification(const FString& Message, bool bIsError, bool bIsWarning)
{
    // 委托到 HUDWidget
    if (UIManager)
    {
        UMAHUDWidget* HUDWidget = UIManager->GetHUDWidget();
        if (HUDWidget)
        {
            HUDWidget->ShowNotification(Message, bIsError, bIsWarning);
        }
    }

    UE_LOG(LogMAHUD, Log, TEXT("ShowNotification: %s (Error=%d, Warning=%d)"),
        *Message, bIsError, bIsWarning);
}

void AMAHUD::ClearHighlightedActor()
{
    AMAPlayerController* MAPC = GetMAPlayerController();
    if (MAPC)
    {
        // 调用 PlayerController 的方法清除高亮
        // 这会触发 OnModifyActorSelected(nullptr)
        MAPC->ClearModifyHighlight();
    }
}

//=============================================================================
// 场景标签可视化
//=============================================================================

void AMAHUD::LoadSceneGraphForVisualization()
{

    // 清空缓存
    CachedSceneNodes.Empty();

    // 获取 SceneGraphManager
    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GI)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("LoadSceneGraphForVisualization: GameInstance not found"));
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("LoadSceneGraphForVisualization: SceneGraphManager not found"));
        return;
    }

    // 获取所有节点
    CachedSceneNodes = SceneGraphManager->GetAllNodes();

    UE_LOG(LogMAHUD, Log, TEXT("LoadSceneGraphForVisualization: Loaded %d nodes"), CachedSceneNodes.Num());
}

void AMAHUD::DrawSceneLabels()
{

    // 检查是否应该显示
    if (!bShowingSceneLabels)
    {
        return;
    }

    // 获取 World 用于 DrawDebugString
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 遍历缓存的节点
    for (const FMASceneGraphNode& Node : CachedSceneNodes)
    {
        // 注意: 实际上 (0,0,0) 可能是有效坐标，但根据设计文档，缺少 center 字段时跳过
        // 这里我们假设如果节点被成功解析，Center 就是有效的

        // 格式化显示文本: "id: [id]\nLabel: [label]"
        // 使用 id 而不是 GUID，因为 id 更简洁易读
        FString IdDisplay = Node.Id.IsEmpty() ? TEXT("N/A") : Node.Id;
        FString LabelDisplay = Node.Label.IsEmpty() ? TEXT("N/A") : Node.Label;
        FString DisplayText = FString::Printf(TEXT("id: %s\nLabel: %s"), *IdDisplay, *LabelDisplay);
        FVector TextPosition = Node.Center + FVector(0.0f, 0.0f, 100.0f);
        // Duration = 0.0f 表示每帧重绘
        DrawDebugString(
            World,
            TextPosition,
            DisplayText,
            nullptr,           // Actor (不附加到任何 Actor)
            FColor::Green,     // 绿色
            0.0f,              // Duration: 每帧重绘
            false              // bDrawShadow
        );
    }
}

void AMAHUD::StartSceneLabelVisualization()
{

    if (bShowingSceneLabels)
    {
        // 已经在显示，只需刷新数据
        LoadSceneGraphForVisualization();
        return;
    }

    // 设置标志
    bShowingSceneLabels = true;

    // 加载场景图数据
    LoadSceneGraphForVisualization();

    UE_LOG(LogMAHUD, Log, TEXT("StartSceneLabelVisualization: Started with %d nodes"), CachedSceneNodes.Num());
}

void AMAHUD::StopSceneLabelVisualization()
{

    if (!bShowingSceneLabels)
    {
        return;
    }

    // 设置标志
    bShowingSceneLabels = false;

    // 清空缓存
    CachedSceneNodes.Empty();

    UE_LOG(LogMAHUD, Log, TEXT("StopSceneLabelVisualization: Stopped"));
}

//=============================================================================
// Edit 模式集成
//=============================================================================

void AMAHUD::BindEditModeManagerEvents()
{

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindEditModeManagerEvents: World not found"));
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindEditModeManagerEvents: EditModeManager not found"));
        return;
    }

    // 绑定选择变化委托
    if (!EditModeManager->OnSelectionChanged.IsAlreadyBound(this, &AMAHUD::OnEditModeSelectionChanged))
    {
        EditModeManager->OnSelectionChanged.AddDynamic(this, &AMAHUD::OnEditModeSelectionChanged);
    }

    UE_LOG(LogMAHUD, Log, TEXT("BindEditModeManagerEvents: Bound to EditModeManager"));
}

void AMAHUD::BindEditWidgetDelegates()
{
    UMAEditWidget* EditWidget = UIManager ? UIManager->GetEditWidget() : nullptr;
    if (!EditWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindEditWidgetDelegates: EditWidget is null"));
        return;
    }

    // 绑定确认修改委托
    if (!EditWidget->OnEditConfirmed.IsAlreadyBound(this, &AMAHUD::OnEditConfirmed))
    {
        EditWidget->OnEditConfirmed.AddDynamic(this, &AMAHUD::OnEditConfirmed);
    }

    // 绑定删除 Actor 委托
    if (!EditWidget->OnDeleteActor.IsAlreadyBound(this, &AMAHUD::OnEditDeleteActor))
    {
        EditWidget->OnDeleteActor.AddDynamic(this, &AMAHUD::OnEditDeleteActor);
    }

    // 绑定创建 Goal 委托
    if (!EditWidget->OnCreateGoal.IsAlreadyBound(this, &AMAHUD::OnEditCreateGoal))
    {
        EditWidget->OnCreateGoal.AddDynamic(this, &AMAHUD::OnEditCreateGoal);
    }

    // 绑定创建 Zone 委托
    if (!EditWidget->OnCreateZone.IsAlreadyBound(this, &AMAHUD::OnEditCreateZone))
    {
        EditWidget->OnCreateZone.AddDynamic(this, &AMAHUD::OnEditCreateZone);
    }

    // 绑定添加预设 Actor 委托
    if (!EditWidget->OnAddPresetActor.IsAlreadyBound(this, &AMAHUD::OnEditAddPresetActor))
    {
        EditWidget->OnAddPresetActor.AddDynamic(this, &AMAHUD::OnEditAddPresetActor);
    }

    // 绑定删除 POI 委托
    if (!EditWidget->OnDeletePOIs.IsAlreadyBound(this, &AMAHUD::OnEditDeletePOIs))
    {
        EditWidget->OnDeletePOIs.AddDynamic(this, &AMAHUD::OnEditDeletePOIs);
    }

    // 绑定设为 Goal 委托
    if (!EditWidget->OnSetAsGoal.IsAlreadyBound(this, &AMAHUD::OnEditSetAsGoal))
    {
        EditWidget->OnSetAsGoal.AddDynamic(this, &AMAHUD::OnEditSetAsGoal);
    }

    // 绑定取消 Goal 委托
    if (!EditWidget->OnUnsetAsGoal.IsAlreadyBound(this, &AMAHUD::OnEditUnsetAsGoal))
    {
        EditWidget->OnUnsetAsGoal.AddDynamic(this, &AMAHUD::OnEditUnsetAsGoal);
    }

    UE_LOG(LogMAHUD, Log, TEXT("BindEditWidgetDelegates: Bound all EditWidget delegates"));
}

void AMAHUD::DrawEditModeIndicator()
{
    // 检查是否在 Edit 模式
    AMAPlayerController* MAPC = GetMAPlayerController();
    bool bInEditMode = MAPC && MAPC->CurrentMouseMode == EMAMouseMode::Edit;

    // 获取 HUDWidget
    UMAHUDWidget* HUDWidget = UIManager ? UIManager->GetHUDWidget() : nullptr;
    
    // 更新 HUDWidget 的 Edit 模式可见性
    if (HUDWidget)
    {
        HUDWidget->SetEditModeVisible(bInEditMode);
    }

    if (!bInEditMode)
    {
        return;
    }

    // 获取 EditModeManager
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    // 收集 POI 数据
    TArray<FString> POIInfos;
    TArray<AMAPointOfInterest*> POIs = EditModeManager->GetAllPOIs();
    for (int32 i = 0; i < POIs.Num(); ++i)
    {
        if (POIs[i])
        {
            FVector Loc = POIs[i]->GetActorLocation();
            POIInfos.Add(FString::Printf(TEXT("[%d](%.0f, %.0f, %.0f)"), i + 1, Loc.X, Loc.Y, Loc.Z));
        }
    }

    // 收集 Goal 数据
    TArray<FString> GoalInfos;
    UGameInstance* GI = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (SceneGraphManager)
    {
        TArray<FMASceneGraphNode> AllGoals = SceneGraphManager->GetAllGoals();
        for (const FMASceneGraphNode& GoalNode : AllGoals)
        {
            FString Label = GoalNode.Label;
            if (Label.IsEmpty())
            {
                Label = GoalNode.Id;
            }

            // 获取 Goal Actor 位置
            AMAGoalActor* GoalActor = EditModeManager->GetGoalActorByNodeId(GoalNode.Id);
            if (GoalActor)
            {
                FVector Loc = GoalActor->GetActorLocation();
                GoalInfos.Add(FString::Printf(TEXT("[%s](%.0f, %.0f, %.0f)"), *Label, Loc.X, Loc.Y, Loc.Z));
            }
            else
            {
                GoalInfos.Add(FString::Printf(TEXT("[%s]"), *Label));
            }
        }

        // 收集 Zone 数据
        TArray<FString> ZoneInfos;
        TArray<FMASceneGraphNode> AllZones = SceneGraphManager->GetAllZones();
        for (const FMASceneGraphNode& ZoneNode : AllZones)
        {
            FString Label = ZoneNode.Label;
            if (Label.IsEmpty())
            {
                Label = ZoneNode.Id;
            }
            ZoneInfos.Add(FString::Printf(TEXT("[%s]"), *Label));
        }

        // 更新 HUDWidget (Canvas 绘制代码已移除)
        if (HUDWidget)
        {
            HUDWidget->UpdatePOIList(POIInfos);
            HUDWidget->UpdateGoalList(GoalInfos);
            HUDWidget->UpdateZoneList(ZoneInfos);
        }
    }
}

void AMAHUD::OnEditModeSelectionChanged()
{

    UE_LOG(LogMAHUD, Log, TEXT("OnEditModeSelectionChanged"));

    UMAEditWidget* EditWidget = UIManager ? UIManager->GetEditWidget() : nullptr;
    if (!EditWidget)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    // 根据选择类型更新 Widget
    if (EditModeManager->HasSelectedActor())
    {
        // 选中了 Actor
        EditWidget->SetSelectedActor(EditModeManager->GetSelectedActor());
    }
    else if (EditModeManager->HasSelectedPOIs())
    {
        // 选中了 POI
        EditWidget->SetSelectedPOIs(EditModeManager->GetSelectedPOIs());
    }
    else
    {
        // 没有选中任何对象
        EditWidget->ClearSelection();
    }
}

void AMAHUD::OnTempSceneGraphChanged()
{

    UE_LOG(LogMAHUD, Log, TEXT("OnTempSceneGraphChanged"));

    UMAEditWidget* EditWidget = UIManager ? UIManager->GetEditWidget() : nullptr;
    if (EditWidget)
    {
        EditWidget->RefreshSceneGraphPreview();
    }
}

void AMAHUD::OnEditConfirmed(AActor* Actor, const FString& JsonContent)
{

    UE_LOG(LogMAHUD, Log, TEXT("OnEditConfirmed: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    if (!Actor)
    {
        ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("Edit failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("Edit failed: EditModeManager not found"), true);
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("Edit failed: SceneGraphManager not found"), true);
        return;
    }

    // 从 JSON 中提取 Node ID
    // 简单解析 JSON 获取 id 字段
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        ShowNotification(TEXT("Invalid JSON format"), true);
        return;
    }

    FString NodeId;
    if (!JsonObject->TryGetStringField(TEXT("id"), NodeId))
    {
        ShowNotification(TEXT("Missing id field in JSON"), true);
        return;
    }

    // 调用 SceneGraphManager 修改 Node (统一 API)
    FString OutError;
    if (!SceneGraphManager->EditNode(NodeId, JsonContent, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }

    // 保存到源文件 (仅 Modify 模式会实际保存)
    SceneGraphManager->SaveToSource();

    // 成功
    ShowNotification(TEXT("Changes saved"), false);

    // 清除选择
    EditModeManager->ClearSelection();
}

void AMAHUD::OnEditDeleteActor(AActor* Actor)
{

    UE_LOG(LogMAHUD, Log, TEXT("OnEditDeleteActor: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    if (!Actor)
    {
        ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("Delete failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("Delete failed: EditModeManager not found"), true);
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("Delete failed: SceneGraphManager not found"), true);
        return;
    }

    FString NodeId;
    bool bIsGoalOrZone = false;

    // 检查是否为 GoalActor 或 ZoneActor，使用 GetNodeId() 获取 Node ID
    if (AMAGoalActor* GoalActor = Cast<AMAGoalActor>(Actor))
    {
        NodeId = GoalActor->GetNodeId();
        bIsGoalOrZone = true;
        UE_LOG(LogMAHUD, Log, TEXT("OnEditDeleteActor: GoalActor detected, NodeId=%s"), *NodeId);
    }
    else if (AMAZoneActor* ZoneActor = Cast<AMAZoneActor>(Actor))
    {
        NodeId = ZoneActor->GetNodeId();
        bIsGoalOrZone = true;
        UE_LOG(LogMAHUD, Log, TEXT("OnEditDeleteActor: ZoneActor detected, NodeId=%s"), *NodeId);
    }

    // 如果是 Goal/Zone Actor，直接使用 NodeId 删除
    if (bIsGoalOrZone)
    {
        if (NodeId.IsEmpty())
        {
            ShowNotification(TEXT("Delete failed: Unable to get Node ID"), true);
            return;
        }

        // 调用 SceneGraphManager 删除 Node (统一 API)
        FString OutError;
        if (!SceneGraphManager->DeleteNode(NodeId, OutError))
        {
            ShowNotification(OutError, true);
            return;
        }

        // 保存到源文件 (仅 Modify 模式会实际保存)
        SceneGraphManager->SaveToSource();

        // 销毁对应的可视化 Actor
        if (Actor->IsA<AMAGoalActor>())
        {
            EditModeManager->DestroyGoalActor(NodeId);
        }
        else if (Actor->IsA<AMAZoneActor>())
        {
            EditModeManager->DestroyZoneActor(NodeId);
        }

        // 成功
        ShowNotification(TEXT("Deleted"), false);

        // 清除选择
        EditModeManager->ClearSelection();
        return;
    }

    // 普通 Actor：通过 SceneGraphManager 查找包含该 GUID 的节点
    FString ActorGuid = Actor->GetActorGuid().ToString();
    TArray<FMASceneGraphNode> MatchingNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);

    if (MatchingNodes.Num() == 0)
    {
        ShowNotification(TEXT("No matching scene graph node found"), true);
        return;
    }

    // 删除第一个匹配的 Node
    FString OutError;
    if (!SceneGraphManager->DeleteNode(MatchingNodes[0].Id, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }

    // 保存到源文件 (仅 Modify 模式会实际保存)
    SceneGraphManager->SaveToSource();

    // 销毁 Actor
    Actor->Destroy();

    // 成功
    ShowNotification(TEXT("Actor deleted"), false);

    // 清除选择
    EditModeManager->ClearSelection();
}

void AMAHUD::OnEditCreateGoal(AMAPointOfInterest* POI, const FString& Description)
{

    UE_LOG(LogMAHUD, Log, TEXT("OnEditCreateGoal: POI=%s, Description=%s"),
        POI ? *POI->GetName() : TEXT("null"), *Description);

    if (!POI)
    {
        ShowNotification(TEXT("Please select a POI first"), true);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("Create failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("Create failed: EditModeManager not found"), true);
        return;
    }

    // 获取 POI 位置
    FVector Location = POI->GetActorLocation();

    // 调用 EditModeManager 创建 Goal
    FString OutError;
    if (!EditModeManager->CreateGoal(Location, Description, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }

    // 销毁 POI
    EditModeManager->DestroyPOI(POI);

    // 成功
    ShowNotification(TEXT("Goal created"), false);

    // 清除选择
    EditModeManager->ClearSelection();
}

void AMAHUD::OnEditCreateZone(const TArray<AMAPointOfInterest*>& POIs, const FString& Description)
{

    UE_LOG(LogMAHUD, Log, TEXT("OnEditCreateZone: %d POIs, Description=%s"), POIs.Num(), *Description);

    if (POIs.Num() < 3)
    {
        ShowNotification(TEXT("Creating a zone requires at least 3 POIs"), true);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("Create failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("Create failed: EditModeManager not found"), true);
        return;
    }

    // 收集 POI 位置
    TArray<FVector> Vertices;
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            Vertices.Add(POI->GetActorLocation());
        }
    }

    if (Vertices.Num() < 3)
    {
        ShowNotification(TEXT("Insufficient valid POIs"), true);
        return;
    }

    // 调用 EditModeManager 创建 Zone
    FString OutError;
    if (!EditModeManager->CreateZone(Vertices, Description, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }

    // 销毁所有参与创建的 POI
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            EditModeManager->DestroyPOI(POI);
        }
    }

    // 成功
    ShowNotification(TEXT("Zone created"), false);

    // 清除选择
    EditModeManager->ClearSelection();
}

void AMAHUD::OnEditAddPresetActor(AMAPointOfInterest* POI, const FString& ActorType)
{

    UE_LOG(LogMAHUD, Log, TEXT("OnEditAddPresetActor: POI=%s, ActorType=%s"),
        POI ? *POI->GetName() : TEXT("null"), *ActorType);

    if (!POI)
    {
        ShowNotification(TEXT("Please select a POI first"), true);
        return;
    }

    if (ActorType.IsEmpty() || ActorType == TEXT("(No preset Actors)"))
    {
        ShowNotification(TEXT("Please select a valid preset Actor type"), true);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("Add failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("Add failed: EditModeManager not found"), true);
        return;
    }

    // TODO: 实现预设 Actor 生成逻辑
    // 当前为占位实现，等待预设 Actor 列表定义

    ShowNotification(TEXT("Preset Actor feature not yet implemented"), false, true);

    UE_LOG(LogMAHUD, Warning, TEXT("OnEditAddPresetActor: Preset Actor feature not yet implemented"));
}

void AMAHUD::OnEditDeletePOIs(const TArray<AMAPointOfInterest*>& POIs)
{
    // 处理删除 POI

    UE_LOG(LogMAHUD, Log, TEXT("OnEditDeletePOIs: %d POIs to delete"), POIs.Num());

    if (POIs.Num() == 0)
    {
        ShowNotification(TEXT("Please select POI first"), true);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("Delete failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("Delete failed: EditModeManager not found"), true);
        return;
    }

    // 删除所有选中的 POI
    int32 DeletedCount = 0;
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            EditModeManager->DestroyPOI(POI);
            DeletedCount++;
        }
    }

    ShowNotification(FString::Printf(TEXT("Deleted %d POIs"), DeletedCount), false);

    UE_LOG(LogMAHUD, Log, TEXT("OnEditDeletePOIs: Deleted %d POIs"), DeletedCount);
}

void AMAHUD::OnEditSetAsGoal(AActor* Actor)
{

    UE_LOG(LogMAHUD, Log, TEXT("OnEditSetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    if (!Actor)
    {
        ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("Set failed: Unable to get World"), true);
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        ShowNotification(TEXT("Set failed: Unable to get GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("Set failed: SceneGraphManager not found"), true);
        return;
    }

    // 通过 GUID 从场景图查找 Node
    FString ActorGuid = Actor->GetActorGuid().ToString();
    TArray<FMASceneGraphNode> MatchingNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);

    if (MatchingNodes.Num() == 0)
    {
        ShowNotification(TEXT("No matching scene graph node found"), true);
        return;
    }

    // 设置第一个匹配的 Node 为 Goal
    FString OutError;
    if (!SceneGraphManager->SetNodeAsGoal(MatchingNodes[0].Id, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }

    // 获取 Node 标签用于显示
    FString NodeLabel = MatchingNodes[0].Label;
    if (NodeLabel.IsEmpty())
    {
        NodeLabel = MatchingNodes[0].Id;
    }

    // 成功
    ShowNotification(FString::Printf(TEXT("Set %s as Goal"), *NodeLabel), false);

    // 刷新 SceneListWidget
    UMASceneListWidget* SceneListWidget = UIManager ? UIManager->GetSceneListWidget() : nullptr;
    if (SceneListWidget)
    {
        SceneListWidget->RefreshLists();
    }

    // 刷新 EditWidget UI 状态
    UMAEditWidget* EditWidget = UIManager ? UIManager->GetEditWidget() : nullptr;
    if (EditWidget)
    {
        EditWidget->SetSelectedActor(Actor);  // 重新设置以刷新按钮状态
    }
}

void AMAHUD::OnEditUnsetAsGoal(AActor* Actor)
{

    UE_LOG(LogMAHUD, Log, TEXT("OnEditUnsetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    if (!Actor)
    {
        ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("Unset failed: Unable to get World"), true);
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        ShowNotification(TEXT("Unset failed: Unable to get GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("Unset failed: SceneGraphManager not found"), true);
        return;
    }

    // 通过 GUID 从场景图查找 Node
    FString ActorGuid = Actor->GetActorGuid().ToString();
    TArray<FMASceneGraphNode> MatchingNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);

    if (MatchingNodes.Num() == 0)
    {
        ShowNotification(TEXT("No matching scene graph node found"), true);
        return;
    }

    // 获取 Node 标签用于显示
    FString NodeLabel = MatchingNodes[0].Label;
    if (NodeLabel.IsEmpty())
    {
        NodeLabel = MatchingNodes[0].Id;
    }

    // 取消第一个匹配的 Node 的 Goal 状态
    FString OutError;
    if (!SceneGraphManager->UnsetNodeAsGoal(MatchingNodes[0].Id, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }

    // 成功
    ShowNotification(FString::Printf(TEXT("Unset %s as Goal"), *NodeLabel), false);

    // 刷新 SceneListWidget
    UMASceneListWidget* SceneListWidget = UIManager ? UIManager->GetSceneListWidget() : nullptr;
    if (SceneListWidget)
    {
        SceneListWidget->RefreshLists();
    }

    // 刷新 EditWidget UI 状态
    UMAEditWidget* EditWidget = UIManager ? UIManager->GetEditWidget() : nullptr;
    if (EditWidget)
    {
        EditWidget->SetSelectedActor(Actor);  // 重新设置以刷新按钮状态
    }
}

void AMAHUD::OnSceneListGoalClicked(const FString& GoalId)
{

    UE_LOG(LogMAHUD, Log, TEXT("OnSceneListGoalClicked: GoalId=%s"), *GoalId);

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    // 获取 Goal Actor
    AMAGoalActor* GoalActor = EditModeManager->GetGoalActorByNodeId(GoalId);
    if (GoalActor)
    {
        // 选中 Goal Actor
        EditModeManager->SelectActor(GoalActor);
        UE_LOG(LogMAHUD, Log, TEXT("OnSceneListGoalClicked: Selected Goal Actor for %s"), *GoalId);
    }
    else
    {
        // 如果没有 Goal Actor，尝试通过 SceneGraphManager 查找对应的场景 Actor
        UGameInstance* GI = World->GetGameInstance();
        if (!GI)
        {
            return;
        }

        UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
        if (!SceneGraphManager)
        {
            return;
        }

        // 获取节点信息
        FMASceneGraphNode Node = SceneGraphManager->GetNodeById(GoalId);
        if (Node.IsValid() && !Node.Guid.IsEmpty())
        {
            // 通过 GUID 查找 Actor
            AActor* FoundActor = EditModeManager->FindActorByGuid(Node.Guid);
            if (FoundActor)
            {
                EditModeManager->SelectActor(FoundActor);
                UE_LOG(LogMAHUD, Log, TEXT("OnSceneListGoalClicked: Selected Actor %s for Goal %s"),
                    *FoundActor->GetName(), *GoalId);
            }
        }
    }
}

void AMAHUD::OnSceneListZoneClicked(const FString& ZoneId)
{

    UE_LOG(LogMAHUD, Log, TEXT("OnSceneListZoneClicked: ZoneId=%s"), *ZoneId);

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    // 获取 Zone Actor
    AMAZoneActor* ZoneActor = EditModeManager->GetZoneActorByNodeId(ZoneId);
    if (ZoneActor)
    {
        // 选中 Zone Actor
        EditModeManager->SelectActor(ZoneActor);
        UE_LOG(LogMAHUD, Log, TEXT("OnSceneListZoneClicked: Selected Zone Actor for %s"), *ZoneId);
    }
    else
    {
        UE_LOG(LogMAHUD, Warning, TEXT("OnSceneListZoneClicked: No Zone Actor found for %s"), *ZoneId);
        ShowNotification(FString::Printf(TEXT("Zone %s has no visualization Actor"), *ZoneId), false, true);
    }
}

//=============================================================================
// 后端事件绑定 (Requirements: 4.1, 4.2, 4.3, 12.1)
//=============================================================================

void AMAHUD::BindBackendEvents()
{
    // 获取 CommSubsystem
    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GI)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindBackendEvents: GameInstance not found"));
        return;
    }

    UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>();
    if (CommSubsystem)
    {
        // 绑定任务图更新事件  (用于 UI 交互流程)
        if (!CommSubsystem->OnTaskPlanReceived.IsAlreadyBound(this, &AMAHUD::OnTaskGraphReceived))
        {
            CommSubsystem->OnTaskPlanReceived.AddDynamic(this, &AMAHUD::OnTaskGraphReceived);
            UE_LOG(LogMAHUD, Log, TEXT("BindBackendEvents: Bound OnTaskPlanReceived"));
        }

        // 绑定技能分配数据更新事件 (用于 UI 交互流程)
        if (!CommSubsystem->OnSkillAllocationReceived.IsAlreadyBound(this, &AMAHUD::OnSkillAllocationReceived))
        {
            CommSubsystem->OnSkillAllocationReceived.AddDynamic(this, &AMAHUD::OnSkillAllocationReceived);
            UE_LOG(LogMAHUD, Log, TEXT("BindBackendEvents: Bound OnSkillAllocationReceived"));
        }

        // 绑定技能列表接收事件 (PLATFORM 类别 - 直接执行，用于 UI 通知)
        if (!CommSubsystem->OnSkillListReceived.IsAlreadyBound(this, &AMAHUD::OnSkillListReceived))
        {
            CommSubsystem->OnSkillListReceived.AddDynamic(this, &AMAHUD::OnSkillListReceived);
            UE_LOG(LogMAHUD, Log, TEXT("BindBackendEvents: Bound OnSkillListReceived"));
        }
    }
    else
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindBackendEvents: CommSubsystem not found"));
    }

    // EmergencyManager 事件已在 BindEmergencyManagerEvents() 中绑定
    // 这里额外绑定到 HUD 状态管理器的通知系统 (Requirements: 4.3, 12.1)
    FMASubsystem Subs = MA_SUBS;
    if (Subs.EmergencyManager && UIManager)
    {
        UMAHUDStateManager* StateManager = UIManager->GetHUDStateManager();
        if (StateManager)
        {
            // 当紧急事件触发时，显示通知
            // 注意：OnEmergencyStateChanged 已在 BindEmergencyManagerEvents 中绑定到 OnEmergencyStateChanged
            // 这里我们在 OnEmergencyStateChanged 回调中处理通知显示
            UE_LOG(LogMAHUD, Log, TEXT("BindBackendEvents: EmergencyManager events will trigger notifications via OnEmergencyStateChanged"));
        }
    }

    // 绑定模态窗口委托 (Requirements: 7.2, 7.3, 12.6)
    BindModalDelegates();

    UE_LOG(LogMAHUD, Log, TEXT("BindBackendEvents: Backend events bound"));
}

void AMAHUD::BindModalDelegates()
{
    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindModalDelegates: UIManager is null"));
        return;
    }

    UMAHUDStateManager* StateManager = UIManager->GetHUDStateManager();
    if (!StateManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindModalDelegates: HUDStateManager is null"));
        return;
    }

    // 绑定模态确认委托 (Requirements: 7.2, 7.3, 12.6)
    if (!StateManager->OnModalConfirmed.IsAlreadyBound(this, &AMAHUD::OnModalConfirmedHandler))
    {
        StateManager->OnModalConfirmed.AddDynamic(this, &AMAHUD::OnModalConfirmedHandler);
        UE_LOG(LogMAHUD, Log, TEXT("BindModalDelegates: Bound OnModalConfirmed"));
    }

    // 绑定模态拒绝委托 (Requirements: 7.2, 7.3, 12.6)
    if (!StateManager->OnModalRejected.IsAlreadyBound(this, &AMAHUD::OnModalRejectedHandler))
    {
        StateManager->OnModalRejected.AddDynamic(this, &AMAHUD::OnModalRejectedHandler);
        UE_LOG(LogMAHUD, Log, TEXT("BindModalDelegates: Bound OnModalRejected"));
    }

    UE_LOG(LogMAHUD, Log, TEXT("BindModalDelegates: Modal delegates bound"));
}

//=============================================================================
// 后端事件回调 (Requirements: 4.1, 4.2, 4.3)
//=============================================================================

void AMAHUD::OnTaskGraphReceived(const FMATaskPlan& TaskPlan)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnTaskGraphReceived: Received task graph update with %d nodes"), TaskPlan.Nodes.Num());

    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("OnTaskGraphReceived: UIManager is null"));
        return;
    }

    // 显示任务图更新通知 (Requirements: 4.1)
    UMAHUDStateManager* StateManager = UIManager->GetHUDStateManager();
    if (StateManager)
    {
        StateManager->ShowNotification(EMANotificationType::TaskGraphUpdate);
        UE_LOG(LogMAHUD, Log, TEXT("OnTaskGraphReceived: Notification shown"));
    }

    // 更新任务图预览 (如果 MainHUDWidget 存在)
    UMAMainHUDWidget* MainHUD = UIManager->GetMainHUDWidget();
    if (MainHUD)
    {
        // 将 TaskPlan 转换为 JSON 用于预览
        FString TaskGraphJson = TaskPlan.ToJson();
        UE_LOG(LogMAHUD, Log, TEXT("OnTaskGraphReceived: Task graph preview update pending, JSON length=%d"), TaskGraphJson.Len());
    }
}

void AMAHUD::OnSkillAllocationReceived(const FMASkillAllocationData& AllocationData)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnSkillAllocationReceived: Received skill allocation data, Name=%s"), *AllocationData.Name);

    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("OnSkillAllocationReceived: UIManager is null"));
        return;
    }

    // 显示技能分配更新通知 (用于 UI 交互流程)
    UMAHUDStateManager* StateManager = UIManager->GetHUDStateManager();
    if (StateManager)
    {
        StateManager->ShowNotification(EMANotificationType::SkillListUpdate);
        UE_LOG(LogMAHUD, Log, TEXT("OnSkillAllocationReceived: Notification shown"));
    }
}


void AMAHUD::OnSkillListReceived(const FMASkillListMessage& SkillList, bool bExecutable)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnSkillListReceived: Received skill list update with %d time steps, executable=%s"), 
        SkillList.TotalTimeSteps, bExecutable ? TEXT("true") : TEXT("false"));

    if (!UIManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("OnSkillListReceived: UIManager is null"));
        return;
    }

    // 显示通知 (Requirements: 4.2)
    UMAHUDStateManager* StateManager = UIManager->GetHUDStateManager();
    if (StateManager)
    {
        if (bExecutable)
        {
            // 可执行的技能列表，显示执行中通知
            StateManager->ShowNotification(EMANotificationType::SkillListExecuting);
        }
        else
        {
            // 普通技能列表更新，显示更新通知
            StateManager->ShowNotification(EMANotificationType::SkillListUpdate);
        }
        UE_LOG(LogMAHUD, Log, TEXT("OnSkillListReceived: Notification shown"));
    }

    // 更新技能列表预览 (如果 MainHUDWidget 存在)
    UMAMainHUDWidget* MainHUD = UIManager->GetMainHUDWidget();
    if (MainHUD)
    {
        // 将 SkillListMessage 转换为 JSON 用于预览
        FString SkillListJson = SkillList.ToJson();
        UE_LOG(LogMAHUD, Log, TEXT("OnSkillListReceived: Skill list preview update pending, JSON length=%d"), SkillListJson.Len());
    }
}

//=============================================================================
// 模态操作回调 (Requirements: 7.2, 7.3, 12.6)
//=============================================================================

void AMAHUD::OnModalConfirmedHandler(EMAModalType ModalType)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnModalConfirmedHandler: Modal confirmed, type=%s"),
        *UEnum::GetValueAsString(ModalType));

    // 根据模态类型执行不同的确认操作
    switch (ModalType)
    {
    case EMAModalType::TaskGraph:
        {
            // 提交任务图到后端 (Requirements: 7.3)
            UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
            if (GI)
            {
                UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>();
                if (CommSubsystem && UIManager)
                {
                    UMATaskGraphModal* TaskGraphModal = UIManager->GetTaskGraphModal();
                    if (TaskGraphModal)
                    {
                        FString TaskGraphJson = TaskGraphModal->GetTaskGraphJson();
                        if (!TaskGraphJson.IsEmpty())
                        {
                            CommSubsystem->SendTaskGraphSubmitMessage(TaskGraphJson);
                            UE_LOG(LogMAHUD, Log, TEXT("OnModalConfirmedHandler: Task graph submitted to backend"));
                        }
                    }
                }
            }
        }
        break;

    case EMAModalType::SkillAllocation:
        {
            // 发送技能分配审阅响应到后端 (Requirements: 5.4)
            UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
            if (GI)
            {
                UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>();
                if (CommSubsystem && UIManager)
                {
                    UMASkillAllocationModal* SkillModal = UIManager->GetSkillAllocationModal();
                    if (SkillModal)
                    {
                        FMASkillAllocationData Data = SkillModal->GetSkillAllocationData();
                        FString ModifiedDataJson = Data.ToJson();
                        CommSubsystem->SendReviewResponseSimple(
                            true,      // bApproved
                            ModifiedDataJson,
                            TEXT("")   // 无拒绝原因
                        );
                        UE_LOG(LogMAHUD, Log, TEXT("OnModalConfirmedHandler: Skill allocation approved"));
                    }
                }
            }
        }
        break;

    case EMAModalType::Emergency:
        {
            // 提交紧急事件响应到后端 (Requirements: 12.6)
            UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
            if (GI)
            {
                UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>();
                if (CommSubsystem && UIManager)
                {
                    UMAEmergencyModal* EmergencyModal = UIManager->GetEmergencyModal();
                    if (EmergencyModal)
                    {
                        FString ResponseJson = EmergencyModal->GetResponseJson();
                        if (!ResponseJson.IsEmpty())
                        {
                            // 发送紧急事件响应
                            FMASceneChangeMessage Message;
                            Message.ChangeType = EMASceneChangeType::EmergencyResponse;
                            Message.Payload = ResponseJson;
                            CommSubsystem->SendSceneChangeMessage(Message);
                            UE_LOG(LogMAHUD, Log, TEXT("OnModalConfirmedHandler: Emergency response submitted to backend"));
                        }
                    }
                }
            }
        }
        break;

    default:
        break;
    }

    // 显示确认通知
    ShowNotification(TEXT("Changes confirmed"), false);
}

void AMAHUD::OnModalRejectedHandler(EMAModalType ModalType)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnModalRejectedHandler: Modal rejected, type=%s"),
        *UEnum::GetValueAsString(ModalType));

    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GI) return;

    UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>();
    if (!CommSubsystem) return;

    // 根据模态类型发送不同的拒绝响应
    switch (ModalType)
    {
    case EMAModalType::SkillAllocation:
        {
            // 发送技能分配审阅拒绝响应 (Requirements: 5.4)
            if (UIManager)
            {
                UMASkillAllocationModal* SkillModal = UIManager->GetSkillAllocationModal();
                if (SkillModal)
                {
                    CommSubsystem->SendReviewResponseSimple(
                        false,     // bApproved
                        TEXT(""),  // 无修改数据
                        TEXT("User rejected the skill allocation")
                    );
                    UE_LOG(LogMAHUD, Log, TEXT("OnModalRejectedHandler: Skill allocation rejected"));
                }
            }
        }
        break;

    default:
        {
            // 其他类型发送通用按钮事件
            FString WidgetName;
            switch (ModalType)
            {
            case EMAModalType::TaskGraph:
                WidgetName = TEXT("TaskGraphModal");
                break;
            case EMAModalType::Emergency:
                WidgetName = TEXT("EmergencyModal");
                break;
            default:
                WidgetName = TEXT("UnknownModal");
                break;
            }
            CommSubsystem->SendButtonEventMessage(WidgetName, TEXT("reject"), TEXT("Reject"));
            UE_LOG(LogMAHUD, Log, TEXT("OnModalRejectedHandler: Reject event sent to backend for %s"), *WidgetName);
        }
        break;
    }

    // 显示拒绝通知
    ShowNotification(TEXT("Changes discarded"), false, true);
}
