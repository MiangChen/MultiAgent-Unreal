// MAHUD.cpp
// HUD 管理器实现
// 继承自 MASelectionHUD 以保留框选绘制功能
// Requirements: 7.1, 2.1, 2.2, 3.3

#include "MAHUD.h"
#include "MASimpleMainWidget.h"
#include "MADirectControlIndicator.h"
#include "MAEmergencyWidget.h"
#include "../Input/MAPlayerController.h"
#include "../Core/MACommSubsystem.h"
#include "../Core/MASimTypes.h"
#include "../Core/MASubsystem.h"
#include "../Core/MAEmergencyManager.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUD, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

AMAHUD::AMAHUD()
{
    // 默认值
    bMainUIVisible = false;
    EmergencyWidget = nullptr;
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
    if (!SimpleMainWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("ShowMainUI: SimpleMainWidget is null"));
        return;
    }

    if (bMainUIVisible)
    {
        // 已经显示，只需重新聚焦
        SimpleMainWidget->FocusInputBox();
        return;
    }

    // 显示 Widget
    SimpleMainWidget->SetVisibility(ESlateVisibility::Visible);
    bMainUIVisible = true;

    // 聚焦到输入框
    SimpleMainWidget->FocusInputBox();

    // 设置输入模式为 UI 和游戏混合
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(SimpleMainWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }

    UE_LOG(LogMAHUD, Log, TEXT("MainUI shown"));
}

void AMAHUD::HideMainUI()
{
    if (!SimpleMainWidget)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("HideMainUI: SimpleMainWidget is null"));
        return;
    }

    if (!bMainUIVisible)
    {
        return;
    }

    // 隐藏 Widget
    SimpleMainWidget->SetVisibility(ESlateVisibility::Collapsed);
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

    // 创建 SimpleMainWidget (纯 C++ UI 实现)
    UE_LOG(LogMAHUD, Log, TEXT("Creating SimpleMainWidget..."));
    
    SimpleMainWidget = CreateWidget<UMASimpleMainWidget>(PC, UMASimpleMainWidget::StaticClass());
    if (SimpleMainWidget)
    {
        SimpleMainWidget->AddToViewport(10);
        SimpleMainWidget->SetVisibility(ESlateVisibility::Collapsed);
        
        // 绑定指令提交事件到 CommSubsystem
        SimpleMainWidget->OnCommandSubmitted.AddDynamic(this, &AMAHUD::OnSimpleCommandSubmitted);
        
        UE_LOG(LogMAHUD, Log, TEXT("SimpleMainWidget created successfully"));
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
}

void AMAHUD::BindControllerEvents()
{
    AMAPlayerController* MAPC = GetMAPlayerController();
    if (!MAPC)
    {
        UE_LOG(LogMAHUD, Warning, TEXT("BindControllerEvents: MAPlayerController not found"));
        return;
    }

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
    
    // 显示响应到 SimpleMainWidget
    if (SimpleMainWidget)
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
