// MASelectionHUD.cpp
// 选择框 HUD 实现

#include "MASelectionHUD.h"
#include "../../Core/Selection/Runtime/MASelectionManager.h"
#include "../../Core/AgentRuntime/Runtime/MAAgentManager.h"
#include "../../Input/MAPlayerController.h"
#include "../../Agent/Character/MACharacter.h"
#include "../Core/MAUIManager.h"
#include "../Core/MAUITheme.h"
#include "MAHUD.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

AMASelectionHUD::AMASelectionHUD()
{
    Theme = nullptr;
}

bool AMASelectionHUD::IsAnyFullscreenWidgetVisible() const
{
    // 获取 PlayerController
    APlayerController* PC = GetOwningPlayerController();
    if (!PC) return false;
    
    // 获取 MAHUD
    AMAHUD* MAHUD = Cast<AMAHUD>(PC->GetHUD());
    if (!MAHUD) return false;
    
    // 获取 UIManager 并检查全屏 Widget
    UMAUIManager* UIManager = MAHUD->GetUIManager();
    if (UIManager)
    {
        return UIManager->IsAnyFullscreenWidgetVisible();
    }
    
    return false;
}

void AMASelectionHUD::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
}

FLinearColor AMASelectionHUD::GetModeColor(EMAMouseMode Mode) const
{
    if (!Theme)
    {
        // Fallback to original hardcoded values
        switch (Mode)
        {
            case EMAMouseMode::Select: return FLinearColor::Green;
            case EMAMouseMode::Deployment: return FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
            case EMAMouseMode::Modify: return FLinearColor(1.0f, 0.6f, 0.0f, 1.0f);
            case EMAMouseMode::Edit: return FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);
            default: return FLinearColor::White;
        }
    }
    
    switch (Mode)
    {
        case EMAMouseMode::Select: return Theme->ModeSelectColor;
        case EMAMouseMode::Deployment: return Theme->ModeDeployColor;
        case EMAMouseMode::Modify: return Theme->ModeModifyColor;
        case EMAMouseMode::Edit: return Theme->ModeEditColor;
        default: return Theme->TextColor;
    }
}

void AMASelectionHUD::DrawHUD()
{
    Super::DrawHUD();

    AMAPlayerController* PC = Cast<AMAPlayerController>(GetOwningPlayerController());

    // 检查是否有全屏 Widget 可见，如果有则不绘制 3D 调试信息
    bool bShouldDrawDebug = !IsAnyFullscreenWidgetVisible();

    // 绘制框选矩形
    if (bIsBoxSelecting)
    {
        // 部署模式用不同颜色
        bool bIsDeploymentMode = PC && PC->IsInDeploymentMode();
        DrawSelectionBox(bIsDeploymentMode);
    }

    // 绘制所有 Agent 的圆圈（仅在没有全屏 Widget 时）
    if (bShouldDrawDebug)
    {
        DrawAllAgentCircles();
        DrawAllAgentStatusText();
    }

    // 绘制编组信息
    DrawControlGroupInfo();

    // 绘制当前鼠标模式
    DrawMouseMode();

    // 绘制部署背包信息（始终显示，如果有内容或在部署模式）
    DrawDeploymentInfo();
}

void AMASelectionHUD::DrawSelectionBox(bool bIsDeploymentMode)
{
    if (!Canvas) return;

    float MinX = FMath::Min(BoxStart.X, BoxEnd.X);
    float MaxX = FMath::Max(BoxStart.X, BoxEnd.X);
    float MinY = FMath::Min(BoxStart.Y, BoxEnd.Y);
    float MaxY = FMath::Max(BoxStart.Y, BoxEnd.Y);

    float Width = MaxX - MinX;
    float Height = MaxY - MinY;

    // 根据模式选择颜色
    FLinearColor FillColor = bIsDeploymentMode ? DeploymentBoxColor : BoxColor;
    FLinearColor BorderColor = bIsDeploymentMode ? DeploymentBoxBorderColor : BoxBorderColor;

    // 绘制半透明填充
    FCanvasTileItem FillItem(
        FVector2D(MinX, MinY),
        FVector2D(Width, Height),
        FillColor
    );
    FillItem.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(FillItem);

    // 绘制边框 (4 条线)
    float BorderThickness = 2.f;
    
    // 上边
    DrawLine(MinX, MinY, MaxX, MinY, BorderColor, BorderThickness);
    // 下边
    DrawLine(MinX, MaxY, MaxX, MaxY, BorderColor, BorderThickness);
    // 左边
    DrawLine(MinX, MinY, MinX, MaxY, BorderColor, BorderThickness);
    // 右边
    DrawLine(MaxX, MinY, MaxX, MaxY, BorderColor, BorderThickness);
}

void AMASelectionHUD::DrawAllAgentCircles()
{
    // 检查是否显示 Agent 圆环
    if (!bShowAgentCircles) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // 获取 AgentManager 以遍历所有 Agent
    UMAAgentManager* AgentManager = World->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;

    // 获取 SelectionManager 以判断选中状态
    UMASelectionManager* SelectionManager = World->GetSubsystem<UMASelectionManager>();

    // 遍历所有 Agent 并绘制圆圈
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            // 根据选中状态选择颜色
            FLinearColor CircleColor;
            if (SelectionManager && SelectionManager->IsSelected(Agent))
            {
                CircleColor = SelectionCircleColor;  // 绿色
            }
            else
            {
                CircleColor = UnselectedCircleColor;  // 橙色
            }
            
            DrawCircleAtAgent(Agent, CircleColor, SelectionCircleRadius);
        }
    }
}

void AMASelectionHUD::DrawControlGroupInfo()
{
    if (!Canvas) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UMASelectionManager* SelectionManager = World->GetSubsystem<UMASelectionManager>();
    if (!SelectionManager) return;

    // 在屏幕左上角显示编组信息
    float StartX = 20.f;
    float StartY = 100.f;
    float LineHeight = 20.f;

    for (int32 i = 1; i <= 5; i++)
    {
        TArray<AMACharacter*> Group = SelectionManager->GetControlGroup(i);
        if (Group.Num() > 0)
        {
            FString Text = FString::Printf(TEXT("[%d] %d units"), i, Group.Num());
            
            // 使用默认字体绘制
            FCanvasTextItem TextItem(
                FVector2D(StartX, StartY + (i - 1) * LineHeight),
                FText::FromString(Text),
                GEngine->GetSmallFont(),
                ControlGroupTextColor
            );
            Canvas->DrawItem(TextItem);
        }
    }
}

void AMASelectionHUD::DrawCircleAtAgent(AMACharacter* Agent, FLinearColor Color, float Radius)
{
    if (!Agent) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // 获取 Agent 中心位置
    FVector AgentLocation = Agent->GetActorLocation();
    float CircleRadius = 60.f;
    FColor DrawColor = Color.ToFColor(true);
    
    // 绘制多个圆环形成球体网格效果 - 使用可配置的线条粗细
    float Thickness = CircleThickness;
    
    // XY 平面 (水平圆 - 赤道)
    DrawDebugCircle(World, AgentLocation, CircleRadius, 24, DrawColor, false, -1.f, 0, Thickness,
        FVector(1, 0, 0), FVector(0, 1, 0), false);
    
    // XZ 平面 (垂直圆 - 前后经线)
    DrawDebugCircle(World, AgentLocation, CircleRadius, 24, DrawColor, false, -1.f, 0, Thickness,
        FVector(1, 0, 0), FVector(0, 0, 1), false);
    
    // YZ 平面 (垂直圆 - 左右经线)
    DrawDebugCircle(World, AgentLocation, CircleRadius, 24, DrawColor, false, -1.f, 0, Thickness,
        FVector(0, 1, 0), FVector(0, 0, 1), false);
    
    // 45度倾斜的经线圆环
    DrawDebugCircle(World, AgentLocation, CircleRadius, 24, DrawColor, false, -1.f, 0, Thickness,
        FVector(0.707f, 0.707f, 0), FVector(0, 0, 1), false);
    
    DrawDebugCircle(World, AgentLocation, CircleRadius, 24, DrawColor, false, -1.f, 0, Thickness,
        FVector(0.707f, -0.707f, 0), FVector(0, 0, 1), false);
    
    // 上下纬线圆环 (北纬/南纬 45度)
    float LatRadius = CircleRadius * 0.707f;  // cos(45°)
    float LatOffset = CircleRadius * 0.707f;  // sin(45°)
    
    DrawDebugCircle(World, AgentLocation + FVector(0, 0, LatOffset), LatRadius, 24, DrawColor, false, -1.f, 0, Thickness,
        FVector(1, 0, 0), FVector(0, 1, 0), false);
    
    DrawDebugCircle(World, AgentLocation - FVector(0, 0, LatOffset), LatRadius, 24, DrawColor, false, -1.f, 0, Thickness,
        FVector(1, 0, 0), FVector(0, 1, 0), false);
}

void AMASelectionHUD::DrawMouseMode()
{
    if (!Canvas) return;

    AMAPlayerController* PC = Cast<AMAPlayerController>(GetOwningPlayerController());
    if (!PC) return;

    // 检查是否有遮挡性 Widget 可见，如果有则不绘制（避免透出）
    // 因为 HUDWidget 已经负责显示 Edit 模式指示器
    UWorld* World = GetWorld();
    if (World)
    {
        TArray<UUserWidget*> FoundWidgets;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, FoundWidgets, UUserWidget::StaticClass(), false);
        for (UUserWidget* Widget : FoundWidgets)
        {
            if (Widget && Widget->IsVisible() && Widget->GetVisibility() == ESlateVisibility::Visible)
            {
                // 排除 HUDWidget（它使用 SelfHitTestInvisible，不会遮挡）
                FString WidgetName = Widget->GetClass()->GetName();
                if (!WidgetName.Contains(TEXT("HUDWidget")))
                {
                    // 有可见的遮挡性 Widget，不绘制模式指示器
                    return;
                }
            }
        }
    }

    // 在屏幕右上角显示当前模式
    FString ModeName = AMAPlayerController::MouseModeToString(PC->GetCurrentMouseMode());
    FString ModeText = FString::Printf(TEXT("Mode: %s (M)"), *ModeName);

    // 计算位置（右上角）
    float TextWidth = 150.f;
    float X = Canvas->SizeX - TextWidth - 20.f;
    float Y = 20.f;

    // 使用 GetModeColor 获取颜色（带 fallback 逻辑）
    FLinearColor ModeColor = GetModeColor(PC->GetCurrentMouseMode());

    FCanvasTextItem TextItem(
        FVector2D(X, Y),
        FText::FromString(ModeText),
        GEngine->GetSmallFont(),
        ModeColor
    );
    Canvas->DrawItem(TextItem);
}

void AMASelectionHUD::DrawDeploymentInfo()
{
    if (!Canvas) return;

    AMAPlayerController* PC = Cast<AMAPlayerController>(GetOwningPlayerController());
    if (!PC) return;

    // 获取背包信息
    TArray<FMAPendingDeployment> Queue = PC->GetDeploymentQueue();
    int32 TotalPending = PC->GetDeploymentQueueCount();
    int32 Deployed = PC->GetDeployedCount();
    bool bInDeploymentMode = PC->IsInDeploymentMode();

    // 如果不在部署模式且背包为空，不显示
    if (!bInDeploymentMode && TotalPending == 0) return;

    // 在屏幕中央上方显示部署信息
    FString InfoText;
    if (bInDeploymentMode)
    {
        InfoText = FString::Printf(TEXT("DEPLOYING: %d remaining"), TotalPending);
    }
    else
    {
        InfoText = FString::Printf(TEXT("Queue: %d units (M to deploy)"), TotalPending);
    }
    
    // 显示待部署列表
    FString PendingText;
    for (int32 i = 0; i < Queue.Num(); ++i)
    {
        if (i > 0) PendingText += TEXT(", ");
        if (bInDeploymentMode && i == 0)
        {
            // 当前正在部署的类型高亮
            PendingText += FString::Printf(TEXT("[%d x %s]"), Queue[i].Count, *Queue[i].AgentType);
        }
        else
        {
            PendingText += FString::Printf(TEXT("%d x %s"), Queue[i].Count, *Queue[i].AgentType);
        }
    }

    float CenterX = Canvas->SizeX / 2.f;
    float Y = 60.f;

    // 绘制背景 - 使用 Theme 颜色（带 fallback）
    float BoxWidth = 450.f;
    float BoxHeight = 55.f;
    FLinearColor BgColor;
    if (Theme)
    {
        BgColor = bInDeploymentMode 
            ? FLinearColor(Theme->ModeDeployColor.R, Theme->ModeDeployColor.G, Theme->ModeDeployColor.B, 0.3f)
            : Theme->BackgroundColor;
    }
    else
    {
        // Fallback to original hardcoded values
        BgColor = bInDeploymentMode 
            ? FLinearColor(0.0f, 0.1f, 0.3f, 0.8f)
            : FLinearColor(0.1f, 0.1f, 0.1f, 0.7f);
    }
    
    FCanvasTileItem BgItem(
        FVector2D(CenterX - BoxWidth / 2.f, Y - 5.f),
        FVector2D(BoxWidth, BoxHeight),
        BgColor
    );
    BgItem.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(BgItem);

    // 绘制文字 - 使用 Theme 颜色（带 fallback）
    FLinearColor InfoColor;
    if (Theme)
    {
        InfoColor = bInDeploymentMode 
            ? Theme->PrimaryColor
            : Theme->SecondaryTextColor;
    }
    else
    {
        // Fallback to original hardcoded values
        InfoColor = bInDeploymentMode 
            ? FLinearColor(0.2f, 0.8f, 1.0f, 1.0f)
            : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
    }
    
    FCanvasTextItem InfoItem(
        FVector2D(CenterX - 180.f, Y),
        FText::FromString(InfoText),
        GEngine->GetMediumFont(),
        InfoColor
    );
    Canvas->DrawItem(InfoItem);

    FCanvasTextItem PendingItem(
        FVector2D(CenterX - 200.f, Y + 22.f),
        FText::FromString(PendingText),
        GEngine->GetSmallFont(),
        FLinearColor::White
    );
    Canvas->DrawItem(PendingItem);
}


void AMASelectionHUD::DrawAllAgentStatusText()
{
    if (!Canvas) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = GetOwningPlayerController();
    if (!PC) return;

    // 获取 AgentManager 以遍历所有 Agent
    UMAAgentManager* AgentManager = World->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;

    // 遍历所有 Agent 并绘制状态文字
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (!Agent) continue;

        // 获取 Agent 的状态文字
        FString StatusText = Agent->GetCurrentStatusText();
        if (StatusText.IsEmpty()) continue;

        // 计算 Agent 头顶位置
        FVector WorldLocation = Agent->GetActorLocation() + FVector(0.f, 0.f, 180.f);

        // 投影到屏幕坐标
        FVector2D ScreenPos;
        if (!PC->ProjectWorldLocationToScreen(WorldLocation, ScreenPos, true))
        {
            continue;  // 不在屏幕内
        }

        // 检查是否在屏幕范围内
        if (ScreenPos.X < 0 || ScreenPos.X > Canvas->SizeX ||
            ScreenPos.Y < 0 || ScreenPos.Y > Canvas->SizeY)
        {
            continue;
        }

        // 绘制黄色大号粗体文字
        FLinearColor TextColor(1.f, 1.f, 0.f, 1.f);  // 黄色
        
        FCanvasTextItem TextItem(
            ScreenPos,
            FText::FromString(StatusText),
            GEngine->GetLargeFont(),
            TextColor
        );
        TextItem.Scale = FVector2D(2.0f, 2.0f);  // 放大 2 倍
        TextItem.bCentreX = true;
        TextItem.bCentreY = true;
        Canvas->DrawItem(TextItem);
    }
}
