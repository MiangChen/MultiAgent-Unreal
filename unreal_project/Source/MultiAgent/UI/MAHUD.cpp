// MAHUD.cpp
// HUD 管理器实现
// 继承自 MASelectionHUD 以保留框选绘制功能
// Requirements: 7.1, 2.1, 2.2, 3.3

#include "MAHUD.h"
#include "MASimpleMainWidget.h"
#include "MATaskPlannerWidget.h"
#include "MADirectControlIndicator.h"
#include "../Core/Types/MATaskGraphTypes.h"
#include "MAEmergencyWidget.h"
#include "MAModifyWidget.h"
#include "MAEditWidget.h"
#include "MASceneListWidget.h"
#include "../Input/MAPlayerController.h"
#include "../Core/Comm/MACommSubsystem.h"
#include "../Core/Types/MASimTypes.h"
#include "../Core/MASubsystem.h"
#include "../Core/Manager/MAEmergencyManager.h"
#include "../Core/Manager/MAEditModeManager.h"
#include "../Core/Manager/MASceneGraphManager.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "../Environment/MAPointOfInterest.h"
#include "../Environment/MAZoneActor.h"
#include "../Environment/MAGoalActor.h"
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
    EmergencyWidget = nullptr;
    TaskPlannerWidget = nullptr;
    ModifyWidget = nullptr;
    EditWidget = nullptr;
    SceneListWidget = nullptr;
}

//=============================================================================
// AHUD 重写
//=============================================================================

void AMAHUD::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogMAHUD, Log, TEXT("MAHUD BeginPlay"));

    // 创建所有 Widget
    CreateWidgets();

    // 绑定 PlayerController 事件
    BindControllerEvents();

    // 绑定 EmergencyManager 事件
    BindEmergencyManagerEvents();

    // 绑定 EditModeManager 事件
    BindEditModeManagerEvents();

    // 绑定 EditWidget 委托
    BindEditWidgetDelegates();
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
    // 优先使用 TaskPlannerWidget
    UUserWidget* ActiveWidget = TaskPlannerWidget ? Cast<UUserWidget>(TaskPlannerWidget) : Cast<UUserWidget>(SimpleMainWidget);
    
    if (!ActiveWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowMainUI: No MainWidget available"));
        return;
    }

    if (bMainUIVisible)
    {
        // 已经显示，只需重新聚焦
        if (TaskPlannerWidget)
        {
            TaskPlannerWidget->FocusJsonEditor();
        }
        else if (SimpleMainWidget)
        {
            SimpleMainWidget->FocusInputBox();
        }
        return;
    }

    // 显示 Widget
    ActiveWidget->SetVisibility(ESlateVisibility::Visible);
    bMainUIVisible = true;

    // 聚焦
    if (TaskPlannerWidget)
    {
        TaskPlannerWidget->FocusJsonEditor();
    }
    else if (SimpleMainWidget)
    {
        SimpleMainWidget->FocusInputBox();
    }

    // 设置输入模式为 UI 和游戏混合
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(ActiveWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }

    UE_LOG(LogMAHUD, Log, TEXT("MainUI shown (TaskPlannerWidget: %s)"), TaskPlannerWidget ? TEXT("Yes") : TEXT("No"));
}

void AMAHUD::HideMainUI()
{
    // 优先使用 TaskPlannerWidget
    UUserWidget* ActiveWidget = TaskPlannerWidget ? Cast<UUserWidget>(TaskPlannerWidget) : Cast<UUserWidget>(SimpleMainWidget);
    
    if (!ActiveWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideMainUI: No MainWidget available"));
        return;
    }

    if (!bMainUIVisible)
    {
        return;
    }

    // 隐藏 Widget
    ActiveWidget->SetVisibility(ESlateVisibility::Collapsed);
    bMainUIVisible = false;

    // 恢复输入模式为纯游戏
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);  // 保持鼠标可见用于 RTS 操作
    }

    UE_LOG(LogMAHUD, Log, TEXT("MainUI hidden"));
}

void AMAHUD::ToggleSemanticMap()
{
    // 后续阶段实现
    if (!SemanticMapWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ToggleSemanticMap: SemanticMapWidget is null"));
        return;
    }

    if (SemanticMapWidget->IsVisible())
    {
        SemanticMapWidget->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAHUD, Log, TEXT("SemanticMap hidden"));
    }
    else
    {
        SemanticMapWidget->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogMAHUD, Log, TEXT("SemanticMap shown"));
    }
}

void AMAHUD::ShowEmergencyWidget()
{
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

    // 显示 Widget
    EmergencyWidget->SetVisibility(ESlateVisibility::Visible);

    // 聚焦到输入框
    EmergencyWidget->FocusInputBox();

    // 设置输入模式为 UI 和游戏混合
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(EmergencyWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }

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

    UE_LOG(LogMAHUD, Log, TEXT("EmergencyWidget shown"));
}

void AMAHUD::HideEmergencyWidget()
{
    if (!EmergencyWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideEmergencyWidget: EmergencyWidget is null"));
        return;
    }

    if (!IsEmergencyWidgetVisible())
    {
        return;
    }

    // 隐藏 Widget
    EmergencyWidget->SetVisibility(ESlateVisibility::Collapsed);

    // 恢复输入模式为纯游戏
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);  // 保持鼠标可见用于 RTS 操作
    }

    UE_LOG(LogMAHUD, Log, TEXT("EmergencyWidget hidden"));
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
    if (!EmergencyWidget)
    {
        return false;
    }
    
    return EmergencyWidget->IsVisible();
}

void AMAHUD::UpdateEmergencyCameraSource(UMACameraSensorComponent* Camera)
{
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
// 突发事件指示器
//=============================================================================

void AMAHUD::ShowEmergencyIndicator()
{
    if (bShowEmergencyIndicator)
    {
        return;  // 已经显示
    }

    bShowEmergencyIndicator = true;
    UE_LOG(LogMAHUD, Log, TEXT("Emergency indicator shown"));
}

void AMAHUD::HideEmergencyIndicator()
{
    if (!bShowEmergencyIndicator)
    {
        return;  // 已经隐藏
    }

    bShowEmergencyIndicator = false;
    UE_LOG(LogMAHUD, Log, TEXT("Emergency indicator hidden"));
}

void AMAHUD::DrawHUD()
{
    Super::DrawHUD();

    // 绘制突发事件指示器
    if (bShowEmergencyIndicator && Canvas)
    {
        // 设置红色字体
        FLinearColor RedColor = FLinearColor::Red;
        
        // 获取屏幕尺寸
        float ScreenWidth = Canvas->SizeX;
        
        // 文字内容
        FString IndicatorText = TEXT("突发事件");
        
        // 计算文字位置 (右上角)
        float TextWidth = IndicatorText.Len() * 20.0f;
        float TextX = ScreenWidth - TextWidth - 30.0f;
        float TextY = 30.0f;
        
        // 绘制红色文字
        FCanvasTextItem TextItem(FVector2D(TextX, TextY), FText::FromString(IndicatorText), GEngine->GetLargeFont(), RedColor);
        TextItem.Scale = FVector2D(1.5f, 1.5f);
        TextItem.bOutlined = true;
        TextItem.OutlineColor = FLinearColor::Black;
        Canvas->DrawItem(TextItem);
    }

    // 绘制场景标签 (Modify 模式)
    DrawSceneLabels();

    // 绘制 Edit 模式指示器和 POI 坐标
    DrawEditModeIndicator();

    // 绘制通知消息
    DrawNotification();
}

void AMAHUD::ShowDirectControlIndicator(AMACharacter* Agent)
{
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
    
    // 显示指示器
    DirectControlIndicator->SetVisibility(ESlateVisibility::Visible);
    
    UE_LOG(LogMAHUD, Log, TEXT("DirectControlIndicator shown for Agent: %s"), *Agent->AgentName);
}

void AMAHUD::HideDirectControlIndicator()
{
    if (!DirectControlIndicator)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideDirectControlIndicator: DirectControlIndicator is null"));
        return;
    }

    DirectControlIndicator->SetVisibility(ESlateVisibility::Collapsed);
    
    UE_LOG(LogMAHUD, Log, TEXT("DirectControlIndicator hidden"));
}

bool AMAHUD::IsDirectControlIndicatorVisible() const
{
    if (!DirectControlIndicator)
    {
        return false;
    }
    
    return DirectControlIndicator->IsVisible();
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

    if (bMainUIVisible && SimpleMainWidget)
    {
        SimpleMainWidget->FocusInputBox();
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
        
        if (IsEmergencyWidgetVisible())
        {
            UpdateEmergencyCameraSource(nullptr);
        }
    }
}

//=============================================================================
// 内部方法
//=============================================================================

void AMAHUD::CreateWidgets()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC)
    {
        UE_LOG(LogMAHUD, Error, TEXT("CreateWidgets: No owning PlayerController"));
        return;
    }

    // 创建 TaskPlannerWidget
    UE_LOG(LogMAHUD, Log, TEXT("Creating TaskPlannerWidget..."));
    
    TaskPlannerWidget = CreateWidget<UMATaskPlannerWidget>(PC, UMATaskPlannerWidget::StaticClass());
    if (TaskPlannerWidget)
    {
        TaskPlannerWidget->AddToViewport(10);
        TaskPlannerWidget->SetVisibility(ESlateVisibility::Collapsed);
        TaskPlannerWidget->OnCommandSubmitted.AddDynamic(this, &AMAHUD::OnSimpleCommandSubmitted);
        
        if (TaskPlannerWidget->IsMockMode())
        {
            TaskPlannerWidget->LoadMockData();
        }
        
        UE_LOG(LogMAHUD, Log, TEXT("TaskPlannerWidget created successfully"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create TaskPlannerWidget"));
    }

    // 创建 SimpleMainWidget (保留向后兼容)
    UE_LOG(LogMAHUD, Log, TEXT("Creating SimpleMainWidget (legacy)..."));
    
    SimpleMainWidget = CreateWidget<UMASimpleMainWidget>(PC, UMASimpleMainWidget::StaticClass());
    if (SimpleMainWidget)
    {
        SimpleMainWidget->SetVisibility(ESlateVisibility::Collapsed);
        SimpleMainWidget->OnCommandSubmitted.AddDynamic(this, &AMAHUD::OnSimpleCommandSubmitted);
        UE_LOG(LogMAHUD, Log, TEXT("SimpleMainWidget created successfully (legacy)"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create SimpleMainWidget"));
    }

    // 创建 SemanticMapWidget (后续阶段)
    if (SemanticMapWidgetClass)
    {
        SemanticMapWidget = CreateWidget<UUserWidget>(PC, SemanticMapWidgetClass);
        if (SemanticMapWidget)
        {
            SemanticMapWidget->AddToViewport(5);
            SemanticMapWidget->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogMAHUD, Log, TEXT("SemanticMapWidget created successfully"));
        }
    }

    // 创建 DirectControlIndicator
    UE_LOG(LogMAHUD, Log, TEXT("Creating DirectControlIndicator..."));
    
    DirectControlIndicator = CreateWidget<UMADirectControlIndicator>(PC, UMADirectControlIndicator::StaticClass());
    if (DirectControlIndicator)
    {
        DirectControlIndicator->AddToViewport(15);
        DirectControlIndicator->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAHUD, Log, TEXT("DirectControlIndicator created successfully"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create DirectControlIndicator"));
    }

    // 创建 EmergencyWidget
    UE_LOG(LogMAHUD, Log, TEXT("Creating EmergencyWidget..."));
    
    EmergencyWidget = CreateWidget<UMAEmergencyWidget>(PC, UMAEmergencyWidget::StaticClass());
    if (EmergencyWidget)
    {
        EmergencyWidget->AddToViewport(12);
        EmergencyWidget->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAHUD, Log, TEXT("EmergencyWidget created successfully"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create EmergencyWidget"));
    }

    // 创建 ModifyWidget
    UE_LOG(LogMAHUD, Log, TEXT("Creating ModifyWidget..."));
    
    ModifyWidget = CreateWidget<UMAModifyWidget>(PC, UMAModifyWidget::StaticClass());
    if (ModifyWidget)
    {
        ModifyWidget->AddToViewport(11);
        ModifyWidget->SetVisibility(ESlateVisibility::Collapsed);
        
        // 绑定委托
        ModifyWidget->OnModifyConfirmed.AddDynamic(this, &AMAHUD::OnModifyConfirmed);
        ModifyWidget->OnMultiSelectModifyConfirmed.AddDynamic(this, &AMAHUD::OnMultiSelectModifyConfirmed);
        
        UE_LOG(LogMAHUD, Log, TEXT("ModifyWidget created successfully"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create ModifyWidget"));
    }

    // 创建 EditWidget
    UE_LOG(LogMAHUD, Log, TEXT("Creating EditWidget..."));
    
    EditWidget = CreateWidget<UMAEditWidget>(PC, UMAEditWidget::StaticClass());
    if (EditWidget)
    {
        EditWidget->AddToViewport(11);
        EditWidget->SetVisibility(ESlateVisibility::Collapsed);
        
        UE_LOG(LogMAHUD, Log, TEXT("EditWidget created successfully"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create EditWidget"));
    }

    // 创建 SceneListWidget
    UE_LOG(LogMAHUD, Log, TEXT("Creating SceneListWidget..."));
    
    SceneListWidget = CreateWidget<UMASceneListWidget>(PC, UMASceneListWidget::StaticClass());
    if (SceneListWidget)
    {
        SceneListWidget->AddToViewport(11);
        SceneListWidget->SetVisibility(ESlateVisibility::Collapsed);
        
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
        
        // 绑定列表项点击委托
        SceneListWidget->OnGoalItemClicked.AddDynamic(this, &AMAHUD::OnSceneListGoalClicked);
        SceneListWidget->OnZoneItemClicked.AddDynamic(this, &AMAHUD::OnSceneListZoneClicked);
        
        UE_LOG(LogMAHUD, Log, TEXT("SceneListWidget created successfully"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create SceneListWidget"));
    }
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

    UE_LOG(LogMAHUD, Log, TEXT("BindControllerEvents: Ready"));
}

void AMAHUD::BindEmergencyManagerEvents()
{
    FMASubsystem Subs = MA_SUBS;
    if (!Subs.EmergencyManager)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindEmergencyManagerEvents: EmergencyManager not found"));
        return;
    }

    Subs.EmergencyManager->OnEmergencyStateChanged.AddDynamic(this, &AMAHUD::OnEmergencyStateChanged);
    
    UE_LOG(LogMAHUD, Log, TEXT("BindEmergencyManagerEvents: Bound to EmergencyManager"));

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
    
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (UMACommSubsystem* CommSubsystem = GI->GetSubsystem<UMACommSubsystem>())
        {
            if (!CommSubsystem->OnPlannerResponse.IsAlreadyBound(this, &AMAHUD::OnPlannerResponse))
            {
                CommSubsystem->OnPlannerResponse.AddDynamic(this, &AMAHUD::OnPlannerResponse);
            }
            
            CommSubsystem->SendNaturalLanguageCommand(Command);
        }
        else
        {
            UE_LOG(LogMAHUD, Warning, TEXT("CommSubsystem not found"));
            if (SimpleMainWidget)
            {
                SimpleMainWidget->SetResultText(TEXT("错误: 通信子系统未找到"));
            }
        }
    }
}

void AMAHUD::OnPlannerResponse(const FMAPlannerResponse& Response)
{
    UE_LOG(LogMAHUD, Log, TEXT("Planner response received: %s"), *Response.Message);
    
    if (TaskPlannerWidget)
    {
        TaskPlannerWidget->AppendStatusLog(FString::Printf(TEXT("[%s] %s"), 
            Response.bSuccess ? TEXT("成功") : TEXT("失败"),
            *Response.Message));
        
        if (!Response.PlanText.IsEmpty())
        {
            TaskPlannerWidget->LoadTaskGraphFromJson(Response.PlanText);
        }
    }
    else if (SimpleMainWidget)
    {
        FString DisplayText = FString::Printf(TEXT("[%s]\n%s"), 
            Response.bSuccess ? TEXT("成功") : TEXT("失败"),
            *Response.Message);
        
        if (!Response.PlanText.IsEmpty())
        {
            DisplayText += FString::Printf(TEXT("\n\n规划:\n%s"), *Response.PlanText);
        }
        
        SimpleMainWidget->SetResultText(DisplayText);
    }
}

void AMAHUD::LoadTaskGraph(const FMATaskGraphData& Data)
{
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
// Modify 模式 UI 控制
//=============================================================================

void AMAHUD::ShowModifyWidget()
{
    if (!ModifyWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowModifyWidget: ModifyWidget is null"));
        return;
    }

    if (IsModifyWidgetVisible())
    {
        return;
    }

    // 显示 Widget
    ModifyWidget->SetVisibility(ESlateVisibility::Visible);

    // 设置输入模式为 UI 和游戏混合
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(ModifyWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }

    // 进入 Modify 模式时开始场景标签可视化
    StartSceneLabelVisualization();

    UE_LOG(LogMAHUD, Log, TEXT("ModifyWidget shown"));
}

void AMAHUD::HideModifyWidget()
{
    if (!ModifyWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideModifyWidget: ModifyWidget is null"));
        return;
    }

    if (!IsModifyWidgetVisible())
    {
        return;
    }

    // 退出 Modify 模式时停止场景标签可视化
    StopSceneLabelVisualization();

    // 隐藏 Widget
    ModifyWidget->SetVisibility(ESlateVisibility::Collapsed);

    // 清除选中状态
    ModifyWidget->ClearSelection();

    // 恢复输入模式为纯游戏
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }

    UE_LOG(LogMAHUD, Log, TEXT("ModifyWidget hidden"));
}

bool AMAHUD::IsModifyWidgetVisible() const
{
    if (!ModifyWidget)
    {
        return false;
    }
    
    return ModifyWidget->IsVisible();
}

void AMAHUD::OnModifyConfirmed(AActor* Actor, const FString& LabelText)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnModifyConfirmed: Actor=%s, LabelText=%s"), 
        Actor ? *Actor->GetName() : TEXT("null"), *LabelText);
    
    // 检查是否有选中的 Actor
    if (!Actor)
    {
        ShowNotification(TEXT("请先选中一个 Actor"), true);
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GI)
    {
        ShowNotification(TEXT("保存失败: 无法获取 GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("保存失败: SceneGraphManager 未找到"), true);
        return;
    }

    // 解析输入
    FString Id, Type, ErrorMessage;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        // 显示错误消息
        ShowNotification(ErrorMessage, true);
        
        // 验证失败时保留输入文本
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActor(Actor);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    // 获取 Actor 世界坐标
    FVector WorldLocation = Actor->GetActorLocation();
    UE_LOG(LogMAHUD, Log, TEXT("OnModifyConfirmed: Actor location = (%f, %f, %f)"), 
        WorldLocation.X, WorldLocation.Y, WorldLocation.Z);

    // 添加场景节点
    FString GeneratedLabel;
    if (!SceneGraphManager->AddSceneNode(Id, Type, WorldLocation, Actor, GeneratedLabel, ErrorMessage))
    {
        // 检查是否是 ID 重复的警告
        if (ErrorMessage.Contains(TEXT("ID 已存在")))
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

    // 显示成功消息
    FString SuccessMessage = FString::Printf(TEXT("标签已保存: %s"), *GeneratedLabel);
    ShowNotification(SuccessMessage, false);

    // 成功添加节点后刷新可视化
    LoadSceneGraphForVisualization();

    // 成功时清空输入并取消选择
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
    
    if (ModifyWidget)
    {
        ModifyWidget->SetSelectedActor(SelectedActor);
    }
}

void AMAHUD::OnModifyActorsSelected(const TArray<AActor*>& SelectedActors)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnModifyActorsSelected: %d actors"), SelectedActors.Num());
    
    if (ModifyWidget)
    {
        ModifyWidget->SetSelectedActors(SelectedActors);
    }
}

void AMAHUD::OnMultiSelectModifyConfirmed(const TArray<AActor*>& Actors, const FString& LabelText, const FString& GeneratedJson)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnMultiSelectModifyConfirmed: %d actors, LabelText=%s"), 
        Actors.Num(), *LabelText);
    
    // 检查是否有选中的 Actor
    if (Actors.Num() == 0)
    {
        ShowNotification(TEXT("请先选中至少一个 Actor"), true);
        return;
    }

    // 获取 SceneGraphManager
    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!GI)
    {
        ShowNotification(TEXT("保存失败: 无法获取 GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("保存失败: SceneGraphManager 未找到"), true);
        return;
    }

    // 解析输入以获取 ID
    FString Id, Type, ErrorMessage;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        ShowNotification(ErrorMessage, true);
        
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
        ShowNotification(FString::Printf(TEXT("ID '%s' 已存在，请使用其他 ID"), *Id), false, true);
        
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActors(Actors);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    UE_LOG(LogMAHUD, Log, TEXT("OnMultiSelectModifyConfirmed: Saving JSON:\n%s"), *GeneratedJson);
    
    // 调用 SceneGraphManager 保存多选节点
    if (!SceneGraphManager->AddMultiSelectNode(GeneratedJson, ErrorMessage))
    {
        ShowNotification(ErrorMessage, true);
        
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActors(Actors);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    // 显示成功消息
    FString SuccessMessage = FString::Printf(TEXT("多选标注已保存: %d 个 Actor"), Actors.Num());
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
    CurrentNotificationMessage = Message;
    
    // 设置颜色
    if (bIsError)
    {
        CurrentNotificationColor = FLinearColor::Red;
    }
    else if (bIsWarning)
    {
        CurrentNotificationColor = FLinearColor::Yellow;
    }
    else
    {
        CurrentNotificationColor = FLinearColor::Green;
    }
    
    bShowNotification = true;
    NotificationStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    
    UE_LOG(LogMAHUD, Log, TEXT("ShowNotification: %s (Error=%d, Warning=%d)"), 
        *Message, bIsError, bIsWarning);
}

void AMAHUD::DrawNotification()
{
    if (!bShowNotification || !Canvas)
    {
        return;
    }
    
    // 检查是否超时
    float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    float ElapsedTime = CurrentTime - NotificationStartTime;
    
    if (ElapsedTime >= NotificationDuration)
    {
        bShowNotification = false;
        return;
    }
    
    // 计算淡出效果 (最后 0.5 秒淡出)
    float Alpha = 1.0f;
    if (ElapsedTime > NotificationDuration - 0.5f)
    {
        Alpha = (NotificationDuration - ElapsedTime) / 0.5f;
    }
    
    // 获取屏幕尺寸
    float ScreenWidth = Canvas->SizeX;
    float ScreenHeight = Canvas->SizeY;
    
    // 计算文字位置 (屏幕底部中央偏上)
    float TextX = ScreenWidth * 0.5f;
    float TextY = ScreenHeight * 0.75f;
    
    // 设置颜色和透明度
    FLinearColor DrawColor = CurrentNotificationColor;
    DrawColor.A = Alpha;
    
    // 绘制通知文字
    FCanvasTextItem TextItem(
        FVector2D(TextX, TextY), 
        FText::FromString(CurrentNotificationMessage), 
        GEngine->GetLargeFont(), 
        DrawColor
    );
    TextItem.Scale = FVector2D(1.2f, 1.2f);
    TextItem.bOutlined = true;
    TextItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, Alpha);
    TextItem.bCentreX = true;
    TextItem.bCentreY = true;
    
    Canvas->DrawItem(TextItem);
}

void AMAHUD::ClearHighlightedActor()
{
    AMAPlayerController* MAPC = GetMAPlayerController();
    if (MAPC)
    {
        // 调用 PlayerController 的方法清除高亮
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
        // 格式化显示文本: "id: [id]\nLabel: [label]"
        FString IdDisplay = Node.Id.IsEmpty() ? TEXT("N/A") : Node.Id;
        FString LabelDisplay = Node.Label.IsEmpty() ? TEXT("N/A") : Node.Label;
        FString DisplayText = FString::Printf(TEXT("id: %s\nLabel: %s"), *IdDisplay, *LabelDisplay);
        
        // 位置: Center + FVector(0, 0, 100) 垂直偏移
        FVector TextPosition = Node.Center + FVector(0.0f, 0.0f, 100.0f);
        
        // 使用 DrawDebugString 绘制绿色文本
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
// Edit 模式 UI 控制
//=============================================================================

void AMAHUD::ShowEditWidget()
{
    if (!EditWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowEditWidget: EditWidget is null"));
        return;
    }

    if (IsEditWidgetVisible())
    {
        return;
    }

    EditWidget->SetVisibility(ESlateVisibility::Visible);

    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(EditWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }

    UE_LOG(LogMAHUD, Log, TEXT("EditWidget shown"));
}

void AMAHUD::HideEditWidget()
{
    if (!EditWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideEditWidget: EditWidget is null"));
        return;
    }

    if (!IsEditWidgetVisible())
    {
        return;
    }

    EditWidget->SetVisibility(ESlateVisibility::Collapsed);
    EditWidget->ClearSelection();

    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }

    UE_LOG(LogMAHUD, Log, TEXT("EditWidget hidden"));
}

bool AMAHUD::IsEditWidgetVisible() const
{
    if (!EditWidget)
    {
        return false;
    }
    return EditWidget->IsVisible();
}

bool AMAHUD::IsMouseOverEditWidget() const
{
    if (!IsEditWidgetVisible())
    {
        return false;
    }
    
    APlayerController* PC = GetOwningPlayerController();
    if (!PC)
    {
        return false;
    }
    
    float MouseX, MouseY;
    if (!PC->GetMousePosition(MouseX, MouseY))
    {
        return false;
    }
    
    int32 ViewportSizeX, ViewportSizeY;
    PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
    
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
    
    if (!EditModeManager->OnSelectionChanged.IsAlreadyBound(this, &AMAHUD::OnEditModeSelectionChanged))
    {
        EditModeManager->OnSelectionChanged.AddDynamic(this, &AMAHUD::OnEditModeSelectionChanged);
    }
    
    if (!EditModeManager->OnTempSceneGraphChanged.IsAlreadyBound(this, &AMAHUD::OnTempSceneGraphChanged))
    {
        EditModeManager->OnTempSceneGraphChanged.AddDynamic(this, &AMAHUD::OnTempSceneGraphChanged);
    }
    
    UE_LOG(LogMAHUD, Log, TEXT("BindEditModeManagerEvents: Bound to EditModeManager"));
}

void AMAHUD::BindEditWidgetDelegates()
{
    if (!EditWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindEditWidgetDelegates: EditWidget is null"));
        return;
    }
    
    if (!EditWidget->OnEditConfirmed.IsAlreadyBound(this, &AMAHUD::OnEditConfirmed))
    {
        EditWidget->OnEditConfirmed.AddDynamic(this, &AMAHUD::OnEditConfirmed);
    }
    
    if (!EditWidget->OnDeleteActor.IsAlreadyBound(this, &AMAHUD::OnEditDeleteActor))
    {
        EditWidget->OnDeleteActor.AddDynamic(this, &AMAHUD::OnEditDeleteActor);
    }
    
    if (!EditWidget->OnCreateGoal.IsAlreadyBound(this, &AMAHUD::OnEditCreateGoal))
    {
        EditWidget->OnCreateGoal.AddDynamic(this, &AMAHUD::OnEditCreateGoal);
    }
    
    if (!EditWidget->OnCreateZone.IsAlreadyBound(this, &AMAHUD::OnEditCreateZone))
    {
        EditWidget->OnCreateZone.AddDynamic(this, &AMAHUD::OnEditCreateZone);
    }
    
    if (!EditWidget->OnAddPresetActor.IsAlreadyBound(this, &AMAHUD::OnEditAddPresetActor))
    {
        EditWidget->OnAddPresetActor.AddDynamic(this, &AMAHUD::OnEditAddPresetActor);
    }
    
    if (!EditWidget->OnDeletePOIs.IsAlreadyBound(this, &AMAHUD::OnEditDeletePOIs))
    {
        EditWidget->OnDeletePOIs.AddDynamic(this, &AMAHUD::OnEditDeletePOIs);
    }
    
    if (!EditWidget->OnSetAsGoal.IsAlreadyBound(this, &AMAHUD::OnEditSetAsGoal))
    {
        EditWidget->OnSetAsGoal.AddDynamic(this, &AMAHUD::OnEditSetAsGoal);
    }
    
    if (!EditWidget->OnUnsetAsGoal.IsAlreadyBound(this, &AMAHUD::OnEditUnsetAsGoal))
    {
        EditWidget->OnUnsetAsGoal.AddDynamic(this, &AMAHUD::OnEditUnsetAsGoal);
    }
    
    UE_LOG(LogMAHUD, Log, TEXT("BindEditWidgetDelegates: Bound all EditWidget delegates"));
}

void AMAHUD::DrawEditModeIndicator()
{
    if (!Canvas)
    {
        return;
    }
    
    AMAPlayerController* MAPC = GetMAPlayerController();
    if (!MAPC || MAPC->CurrentMouseMode != EMAMouseMode::Edit)
    {
        return;
    }
    
    float ScreenWidth = Canvas->SizeX;
    float ScreenHeight = Canvas->SizeY;
    
    FString ModeText = TEXT("Mode: Edit (M)");
    float ModeTextWidth = ModeText.Len() * 10.0f;
    float ModeTextX = ScreenWidth - ModeTextWidth - 30.0f;
    float ModeTextY = 30.0f;
    
    FLinearColor BlueColor = FLinearColor(0.3f, 0.6f, 1.0f);
    
    FCanvasTextItem ModeTextItem(FVector2D(ModeTextX, ModeTextY), FText::FromString(ModeText), GEngine->GetLargeFont(), BlueColor);
    ModeTextItem.Scale = FVector2D(1.2f, 1.2f);
    ModeTextItem.bOutlined = true;
    ModeTextItem.OutlineColor = FLinearColor::Black;
    Canvas->DrawItem(ModeTextItem);
    
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
    
    float CurrentY = ScreenHeight - 30.0f;
    const float LineHeight = 18.0f;
    
    FLinearColor GreenColor = FLinearColor(0.3f, 0.8f, 0.3f);
    FLinearColor RedColor = FLinearColor(1.0f, 0.4f, 0.4f);
    FLinearColor CyanColor = FLinearColor(0.3f, 0.8f, 1.0f);
    
    TArray<AMAPointOfInterest*> POIs = EditModeManager->GetAllPOIs();
    if (POIs.Num() > 0)
    {
        FString POIText = TEXT("POI: ");
        for (int32 i = 0; i < POIs.Num(); ++i)
        {
            if (POIs[i])
            {
                FVector Loc = POIs[i]->GetActorLocation();
                POIText += FString::Printf(TEXT("[%d](%.0f, %.0f, %.0f) "), i + 1, Loc.X, Loc.Y, Loc.Z);
            }
        }
        
        FCanvasTextItem POITextItem(FVector2D(20.0f, CurrentY), FText::FromString(POIText), GEngine->GetSmallFont(), GreenColor);
        POITextItem.Scale = FVector2D(1.0f, 1.0f);
        POITextItem.bOutlined = true;
        POITextItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
        Canvas->DrawItem(POITextItem);
        CurrentY -= LineHeight;
    }
}

void AMAHUD::OnEditModeSelectionChanged()
{
    UE_LOG(LogMAHUD, Log, TEXT("OnEditModeSelectionChanged"));
    
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
    
    if (EditModeManager->HasSelectedActor())
    {
        EditWidget->SetSelectedActor(EditModeManager->GetSelectedActor());
    }
    else if (EditModeManager->HasSelectedPOIs())
    {
        EditWidget->SetSelectedPOIs(EditModeManager->GetSelectedPOIs());
    }
    else
    {
        EditWidget->ClearSelection();
    }
}

void AMAHUD::OnTempSceneGraphChanged()
{
    UE_LOG(LogMAHUD, Log, TEXT("OnTempSceneGraphChanged"));
    
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
        ShowNotification(TEXT("请先选中一个 Actor"), true);
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("修改失败: 无法获取 World"), true);
        return;
    }
    
    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("修改失败: EditModeManager 未找到"), true);
        return;
    }
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        ShowNotification(TEXT("JSON 格式无效"), true);
        return;
    }
    
    FString NodeId;
    if (!JsonObject->TryGetStringField(TEXT("id"), NodeId))
    {
        ShowNotification(TEXT("JSON 中缺少 id 字段"), true);
        return;
    }
    
    FString OutError;
    if (!EditModeManager->EditNode(NodeId, JsonContent, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }
    
    ShowNotification(TEXT("修改已保存"), false);
    EditModeManager->ClearSelection();
}

void AMAHUD::OnEditDeleteActor(AActor* Actor)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnEditDeleteActor: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));
    
    if (!Actor)
    {
        ShowNotification(TEXT("请先选中一个 Actor"), true);
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("删除失败: 无法获取 World"), true);
        return;
    }
    
    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("删除失败: EditModeManager 未找到"), true);
        return;
    }
    
    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        ShowNotification(TEXT("删除失败: 无法获取 GameInstance"), true);
        return;
    }
    
    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("删除失败: SceneGraphManager 未找到"), true);
        return;
    }
    
    FString ActorGuid = Actor->GetActorGuid().ToString();
    TArray<FMASceneGraphNode> Nodes = SceneGraphManager->FindNodesByGuid(ActorGuid);
    
    if (Nodes.Num() == 0)
    {
        ShowNotification(TEXT("未找到对应的场景图节点"), true);
        return;
    }
    
    FString OutError;
    if (!EditModeManager->DeleteNode(Nodes[0].Id, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }
    
    Actor->Destroy();
    ShowNotification(TEXT("Actor 已删除"), false);
    EditModeManager->ClearSelection();
}

void AMAHUD::OnEditCreateGoal(AMAPointOfInterest* POI, const FString& Description)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnEditCreateGoal: POI=%s, Description=%s"), 
        POI ? *POI->GetName() : TEXT("null"), *Description);
    
    if (!POI)
    {
        ShowNotification(TEXT("请先选中一个 POI"), true);
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("创建失败: 无法获取 World"), true);
        return;
    }
    
    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("创建失败: EditModeManager 未找到"), true);
        return;
    }
    
    FVector Location = POI->GetActorLocation();
    
    FString OutError;
    if (!EditModeManager->CreateGoal(Location, Description, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }
    
    EditModeManager->DestroyPOI(POI);
    ShowNotification(TEXT("Goal 已创建"), false);
    EditModeManager->ClearSelection();
}

void AMAHUD::OnEditCreateZone(const TArray<AMAPointOfInterest*>& POIs, const FString& Description)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnEditCreateZone: %d POIs, Description=%s"), POIs.Num(), *Description);
    
    if (POIs.Num() < 3)
    {
        ShowNotification(TEXT("创建区域需要至少 3 个 POI"), true);
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("创建失败: 无法获取 World"), true);
        return;
    }
    
    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("创建失败: EditModeManager 未找到"), true);
        return;
    }
    
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
        ShowNotification(TEXT("有效 POI 数量不足"), true);
        return;
    }
    
    FString OutError;
    if (!EditModeManager->CreateZone(Vertices, Description, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }
    
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            EditModeManager->DestroyPOI(POI);
        }
    }
    
    ShowNotification(TEXT("区域已创建"), false);
    EditModeManager->ClearSelection();
}

void AMAHUD::OnEditAddPresetActor(AMAPointOfInterest* POI, const FString& ActorType)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnEditAddPresetActor: POI=%s, ActorType=%s"), 
        POI ? *POI->GetName() : TEXT("null"), *ActorType);
    
    if (!POI)
    {
        ShowNotification(TEXT("请先选中一个 POI"), true);
        return;
    }
    
    if (ActorType.IsEmpty() || ActorType == TEXT("(暂无预设 Actor)"))
    {
        ShowNotification(TEXT("请选择一个有效的预设 Actor 类型"), true);
        return;
    }
    
    ShowNotification(TEXT("预设 Actor 功能暂未实现"), false, true);
    UE_LOG(LogMAHUD, Warning, TEXT("OnEditAddPresetActor: Preset Actor feature not yet implemented"));
}

void AMAHUD::OnEditDeletePOIs(const TArray<AMAPointOfInterest*>& POIs)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnEditDeletePOIs: %d POIs to delete"), POIs.Num());
    
    if (POIs.Num() == 0)
    {
        ShowNotification(TEXT("请先选中 POI"), true);
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("删除失败: 无法获取 World"), true);
        return;
    }
    
    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("删除失败: EditModeManager 未找到"), true);
        return;
    }
    
    int32 DeletedCount = 0;
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            EditModeManager->DestroyPOI(POI);
            DeletedCount++;
        }
    }
    
    ShowNotification(FString::Printf(TEXT("已删除 %d 个 POI"), DeletedCount), false);
}

void AMAHUD::OnEditSetAsGoal(AActor* Actor)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnEditSetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));
    
    if (!Actor)
    {
        ShowNotification(TEXT("请先选中一个 Actor"), true);
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("设置失败: 无法获取 World"), true);
        return;
    }
    
    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("设置失败: EditModeManager 未找到"), true);
        return;
    }
    
    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        ShowNotification(TEXT("设置失败: 无法获取 GameInstance"), true);
        return;
    }
    
    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("设置失败: SceneGraphManager 未找到"), true);
        return;
    }
    
    FString ActorGuid = Actor->GetActorGuid().ToString();
    TArray<FMASceneGraphNode> Nodes = SceneGraphManager->FindNodesByGuid(ActorGuid);
    
    if (Nodes.Num() == 0)
    {
        ShowNotification(TEXT("未找到对应的场景图节点"), true);
        return;
    }
    
    FString OutError;
    if (!EditModeManager->SetNodeAsGoal(Nodes[0].Id, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }
    
    ShowNotification(FString::Printf(TEXT("已将 %s 设为 Goal"), *Nodes[0].Label), false);
    
    if (SceneListWidget)
    {
        SceneListWidget->RefreshLists();
    }
    
    if (EditWidget)
    {
        EditWidget->SetSelectedActor(Actor);
    }
}

void AMAHUD::OnEditUnsetAsGoal(AActor* Actor)
{
    UE_LOG(LogMAHUD, Log, TEXT("OnEditUnsetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));
    
    if (!Actor)
    {
        ShowNotification(TEXT("请先选中一个 Actor"), true);
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        ShowNotification(TEXT("取消失败: 无法获取 World"), true);
        return;
    }
    
    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        ShowNotification(TEXT("取消失败: EditModeManager 未找到"), true);
        return;
    }
    
    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        ShowNotification(TEXT("取消失败: 无法获取 GameInstance"), true);
        return;
    }
    
    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        ShowNotification(TEXT("取消失败: SceneGraphManager 未找到"), true);
        return;
    }
    
    FString ActorGuid = Actor->GetActorGuid().ToString();
    TArray<FMASceneGraphNode> Nodes = SceneGraphManager->FindNodesByGuid(ActorGuid);
    
    if (Nodes.Num() == 0)
    {
        ShowNotification(TEXT("未找到对应的场景图节点"), true);
        return;
    }
    
    FString OutError;
    if (!EditModeManager->UnsetNodeAsGoal(Nodes[0].Id, OutError))
    {
        ShowNotification(OutError, true);
        return;
    }
    
    ShowNotification(FString::Printf(TEXT("已取消 %s 的 Goal 状态"), *Nodes[0].Label), false);
    
    if (SceneListWidget)
    {
        SceneListWidget->RefreshLists();
    }
    
    if (EditWidget)
    {
        EditWidget->SetSelectedActor(Actor);
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
    
    AMAGoalActor* GoalActor = EditModeManager->GetGoalActorByNodeId(GoalId);
    if (GoalActor)
    {
        EditModeManager->SelectActor(GoalActor);
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
    
    AMAZoneActor* ZoneActor = EditModeManager->GetZoneActorByNodeId(ZoneId);
    if (ZoneActor)
    {
        EditModeManager->SelectActor(ZoneActor);
    }
    else
    {
        ShowNotification(FString::Printf(TEXT("Zone %s 没有可视化 Actor"), *ZoneId), false, true);
    }
}
