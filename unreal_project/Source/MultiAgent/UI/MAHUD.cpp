// MAHUD.cpp
// HUD 管理器实现
// 继承自 MASelectionHUD 以保留框选绘制功能
// Requirements: 7.1, 2.1, 2.2, 3.3

#include "MAHUD.h"
#include "MASimpleMainWidget.h"
#include "MATaskPlannerWidget.h"
#include "MADirectControlIndicator.h"
#include "../Core/MATaskGraphTypes.h"
#include "MAEmergencyWidget.h"
#include "MAModifyWidget.h"
#include "../Input/MAPlayerController.h"
#include "../Core/MACommSubsystem.h"
#include "../Core/MASimTypes.h"
#include "../Core/MASubsystem.h"
#include "../Core/MAEmergencyManager.h"
#include "../Core/MASceneGraphManager.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"
#include "DrawDebugHelpers.h"

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
        // 使用默认字体大小估算文字宽度
        float TextWidth = IndicatorText.Len() * 20.0f;  // 估算每个中文字符约 20 像素
        float TextX = ScreenWidth - TextWidth - 30.0f;  // 右边距 30 像素
        float TextY = 30.0f;  // 上边距 30 像素
        
        // 绘制红色文字
        FCanvasTextItem TextItem(FVector2D(TextX, TextY), FText::FromString(IndicatorText), GEngine->GetLargeFont(), RedColor);
        TextItem.Scale = FVector2D(1.5f, 1.5f);  // 放大 1.5 倍
        TextItem.bOutlined = true;  // 添加轮廓使文字更清晰
        TextItem.OutlineColor = FLinearColor::Black;
        Canvas->DrawItem(TextItem);
    }

    // 绘制场景标签 (Modify 模式)
    // Requirements: 5.3 - 每帧刷新显示
    DrawSceneLabels();

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

void AMAHUD::CreateWidgets()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC)
    {
        UE_LOG(LogMAHUD, Error, TEXT("CreateWidgets: No owning PlayerController"));
        return;
    }

    // 创建 TaskPlannerWidget (任务规划工作台 - 替代 SimpleMainWidget)
    UE_LOG(LogMAHUD, Log, TEXT("Creating TaskPlannerWidget..."));
    
    TaskPlannerWidget = CreateWidget<UMATaskPlannerWidget>(PC, UMATaskPlannerWidget::StaticClass());
    if (TaskPlannerWidget)
    {
        TaskPlannerWidget->AddToViewport(10);
        TaskPlannerWidget->SetVisibility(ESlateVisibility::Collapsed);
        
        // 绑定指令提交事件到 CommSubsystem
        TaskPlannerWidget->OnCommandSubmitted.AddDynamic(this, &AMAHUD::OnSimpleCommandSubmitted);
        
        // 尝试加载 Mock 数据
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

    // 创建 SimpleMainWidget (保留向后兼容，但默认不使用)
    UE_LOG(LogMAHUD, Log, TEXT("Creating SimpleMainWidget (legacy)..."));
    
    SimpleMainWidget = CreateWidget<UMASimpleMainWidget>(PC, UMASimpleMainWidget::StaticClass());
    if (SimpleMainWidget)
    {
        // 不添加到 Viewport，仅保留引用
        // SimpleMainWidget->AddToViewport(10);
        SimpleMainWidget->SetVisibility(ESlateVisibility::Collapsed);
        
        // 绑定指令提交事件到 CommSubsystem
        SimpleMainWidget->OnCommandSubmitted.AddDynamic(this, &AMAHUD::OnSimpleCommandSubmitted);
        
        UE_LOG(LogMAHUD, Log, TEXT("SimpleMainWidget created successfully (legacy, not added to viewport)"));
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

    // 创建 DirectControlIndicator (Requirements: 3.3)
    UE_LOG(LogMAHUD, Log, TEXT("Creating DirectControlIndicator..."));
    
    DirectControlIndicator = CreateWidget<UMADirectControlIndicator>(PC, UMADirectControlIndicator::StaticClass());
    if (DirectControlIndicator)
    {
        DirectControlIndicator->AddToViewport(15);  // 高优先级，显示在其他 UI 之上
        DirectControlIndicator->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAHUD, Log, TEXT("DirectControlIndicator created successfully"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create DirectControlIndicator"));
    }

    // 创建 EmergencyWidget (Requirements: 2.1, 2.4, 2.5, 4.3)
    UE_LOG(LogMAHUD, Log, TEXT("Creating EmergencyWidget..."));
    
    EmergencyWidget = CreateWidget<UMAEmergencyWidget>(PC, UMAEmergencyWidget::StaticClass());
    if (EmergencyWidget)
    {
        EmergencyWidget->AddToViewport(12);  // 中等优先级，在 MainWidget 之上但在 DirectControlIndicator 之下
        EmergencyWidget->SetVisibility(ESlateVisibility::Collapsed);
        UE_LOG(LogMAHUD, Log, TEXT("EmergencyWidget created successfully"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create EmergencyWidget"));
    }

    // 创建 ModifyWidget (Requirements: 3.1, 3.2, 3.3)
    UE_LOG(LogMAHUD, Log, TEXT("Creating ModifyWidget..."));
    
    ModifyWidget = CreateWidget<UMAModifyWidget>(PC, UMAModifyWidget::StaticClass());
    if (ModifyWidget)
    {
        ModifyWidget->AddToViewport(11);  // 中等优先级，在 MainWidget 之上
        ModifyWidget->SetVisibility(ESlateVisibility::Collapsed);
        
        // 绑定 OnModifyConfirmed 委托 (单选模式)
        ModifyWidget->OnModifyConfirmed.AddDynamic(this, &AMAHUD::OnModifyConfirmed);
        
        // 绑定 OnMultiSelectModifyConfirmed 委托 (多选模式)
        ModifyWidget->OnMultiSelectModifyConfirmed.AddDynamic(this, &AMAHUD::OnMultiSelectModifyConfirmed);
        
        UE_LOG(LogMAHUD, Log, TEXT("ModifyWidget created successfully"));
    }
    else
    {
        UE_LOG(LogMAHUD, Error, TEXT("Failed to create ModifyWidget"));
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

    // 注意: 这里需要 MAPlayerController 定义 OnMainUIToggled 和 OnRefocusMainUI 委托
    // 这些委托将在后续任务 4.2 中添加到 MAPlayerController
    // 目前先记录日志，等待 MAPlayerController 扩展后再绑定

    UE_LOG(LogMAHUD, Log, TEXT("BindControllerEvents: Ready to bind when MAPlayerController delegates are available"));
    
    // TODO: 在任务 5.2 中完成绑定
    // MAPC->OnMainUIToggled.AddDynamic(this, &AMAHUD::OnMainUIToggled);
    // MAPC->OnRefocusMainUI.AddDynamic(this, &AMAHUD::OnRefocusMainUI);
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
    
    // 显示响应到 TaskPlannerWidget 或 SimpleMainWidget
    if (TaskPlannerWidget)
    {
        // 追加状态日志
        TaskPlannerWidget->AppendStatusLog(FString::Printf(TEXT("[%s] %s"), 
            Response.bSuccess ? TEXT("成功") : TEXT("失败"),
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
        // 已经显示
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

    // Requirements: 5.1 - 进入 Modify 模式时开始场景标签可视化
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

    // Requirements: 5.2 - 退出 Modify 模式时停止场景标签可视化
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
        PC->SetShowMouseCursor(true);  // 保持鼠标可见用于 RTS 操作
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
    
    // Requirements: 5.1 - 检查是否有选中的 Actor
    if (!Actor)
    {
        ShowNotification(TEXT("请先选中一个 Actor"), true);
        return;
    }

    // 获取 SceneGraphManager
    // Requirements: 8.2, 8.3
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

    // Requirements: 1.1, 1.2, 1.3, 1.4 - 解析输入
    FString Id, Type, ErrorMessage;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        // Requirements: 2.1, 2.2, 2.3 - 显示错误消息
        ShowNotification(ErrorMessage, true);
        
        // Requirements: 2.5 - 验证失败时保留输入文本
        // ModifyWidget 不清空，由 ModifyWidget 自己处理
        // 重新设置选中的 Actor 以保留状态
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActor(Actor);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    // Requirements: 5.1, 5.3 - 获取 Actor 世界坐标
    FVector WorldLocation = Actor->GetActorLocation();
    UE_LOG(LogMAHUD, Log, TEXT("OnModifyConfirmed: Actor location = (%f, %f, %f)"), 
        WorldLocation.X, WorldLocation.Y, WorldLocation.Z);

    // Requirements: 3.1, 3.2, 6.3, 6.5, 7.1, 7.2, 7.3, 7.4 - 添加场景节点
    FString GeneratedLabel;
    if (!SceneGraphManager->AddSceneNode(Id, Type, WorldLocation, Actor, GeneratedLabel, ErrorMessage))
    {
        // Requirements: 3.2 - ID 重复或其他错误
        // 检查是否是 ID 重复的警告
        if (ErrorMessage.Contains(TEXT("ID 已存在")))
        {
            ShowNotification(ErrorMessage, false, true);  // 警告样式
            // Requirements: 2.5 - ID 重复时清空输入（因为用户需要输入新的 ID）
            if (ModifyWidget)
            {
                ModifyWidget->ClearSelection();
            }
            ClearHighlightedActor();
        }
        else
        {
            ShowNotification(ErrorMessage, true);  // 错误样式
            // Requirements: 2.5 - 其他错误时保留输入文本
            if (ModifyWidget)
            {
                ModifyWidget->SetSelectedActor(Actor);
                ModifyWidget->SetLabelText(LabelText);
            }
        }
        return;
    }

    // Requirements: 6.5 - 显示成功消息
    FString SuccessMessage = FString::Printf(TEXT("标签已保存: %s"), *GeneratedLabel);
    ShowNotification(SuccessMessage, false);

    // Requirements: 5.4 - 成功添加节点后刷新可视化
    // 重新加载场景图数据以显示新添加的节点
    LoadSceneGraphForVisualization();

    // Requirements: 2.5 - 成功时清空输入并取消选择
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
    
    // Requirements: 8.1 - 检查是否有选中的 Actor
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
        ShowNotification(FString::Printf(TEXT("ID '%s' 已存在，请使用其他 ID"), *Id), false, true);
        
        // 保留输入状态
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActors(Actors);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    // TODO: 将生成的 JSON 保存到 SceneGraphManager
    // 目前 SceneGraphManager 只支持单节点添加，需要扩展以支持多选节点
    // 这里先输出日志，后续任务 8 会完成集成
    UE_LOG(LogMAHUD, Log, TEXT("OnMultiSelectModifyConfirmed: Saving JSON:\n%s"), *GeneratedJson);
    
    // 调用 SceneGraphManager 保存多选节点
    if (!SceneGraphManager->AddMultiSelectNode(GeneratedJson, ErrorMessage))
    {
        // 保存失败
        ShowNotification(ErrorMessage, true);
        
        // 保留输入状态
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
// Requirements: 2.1, 3.2, 6.5
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
        // 这会触发 OnModifyActorSelected(nullptr)
        MAPC->ClearModifyHighlight();
    }
}

//=============================================================================
// 场景标签可视化
// Requirements: 3.1, 3.2, 3.3, 3.4, 4.1, 4.2, 4.3, 4.4, 4.5, 5.1, 5.2, 5.3, 6.1
//=============================================================================

void AMAHUD::LoadSceneGraphForVisualization()
{
    // Requirements: 3.1 - 读取场景图数据
    // Requirements: 3.2 - 如果文件不存在，跳过可视化
    // Requirements: 3.3 - 如果数据无效，记录警告并跳过
    
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
    // Requirements: 4.1 - 在 Modify 模式激活时显示绿色文本
    // Requirements: 5.3 - 每帧刷新显示
    
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
        // Requirements: 4.3 - 跳过缺少 center 的节点 (Center 为零向量表示无效)
        // 注意: 实际上 (0,0,0) 可能是有效坐标，但根据设计文档，缺少 center 字段时跳过
        // 这里我们假设如果节点被成功解析，Center 就是有效的
        
        // 格式化显示文本: "id: [id]\nLabel: [label]"
        // 使用 id 而不是 GUID，因为 id 更简洁易读
        FString IdDisplay = Node.Id.IsEmpty() ? TEXT("N/A") : Node.Id;
        FString LabelDisplay = Node.Label.IsEmpty() ? TEXT("N/A") : Node.Label;
        FString DisplayText = FString::Printf(TEXT("id: %s\nLabel: %s"), *IdDisplay, *LabelDisplay);
        
        // Requirements: 4.5 - 位置: Center + FVector(0, 0, 100) 垂直偏移
        FVector TextPosition = Node.Center + FVector(0.0f, 0.0f, 100.0f);
        
        // Requirements: 4.4 - 使用 DrawDebugString 绘制绿色文本
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
    // Requirements: 5.1 - 进入 Modify 模式时开始显示标签
    
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
    // Requirements: 5.2 - 退出 Modify 模式时停止显示标签
    
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
