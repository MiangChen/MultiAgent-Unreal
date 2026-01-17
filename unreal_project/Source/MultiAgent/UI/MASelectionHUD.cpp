// MASelectionHUD.cpp
// 选择框 HUD 实现

#include "MASelectionHUD.h"
#include "../Core/Manager/MASelectionManager.h"
#include "../Input/MAPlayerController.h"
#include "../Agent/Character/MACharacter.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

AMASelectionHUD::AMASelectionHUD()
{
}

void AMASelectionHUD::DrawHUD()
{
    Super::DrawHUD();

    AMAPlayerController* PC = Cast<AMAPlayerController>(GetOwningPlayerController());

    // 绘制框选矩形
    if (bIsBoxSelecting)
    {
        // 部署模式用不同颜色
        bool bIsDeploymentMode = PC && PC->IsInDeploymentMode();
        DrawSelectionBox(bIsDeploymentMode);
    }

    // 绘制选中 Agent 的高亮
    DrawSelectedAgents();

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

void AMASelectionHUD::DrawSelectedAgents()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UMASelectionManager* SelectionManager = World->GetSubsystem<UMASelectionManager>();
    if (!SelectionManager) return;

    for (AMACharacter* Agent : SelectionManager->GetSelectedAgents())
    {
        if (Agent)
        {
            DrawCircleAtAgent(Agent, SelectionCircleColor, SelectionCircleRadius);
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
    
    // 绘制 3 个互相垂直的圆环
    // XY 平面 (水平圆)
    DrawDebugCircle(
        World,
        AgentLocation,
        CircleRadius,
        24,             // Segments
        DrawColor,
        false,          // bPersistentLines
        -1.f,           // LifeTime
        0,              // DepthPriority
        2.f,            // Thickness
        FVector(1, 0, 0),  // YAxis
        FVector(0, 1, 0),  // ZAxis
        false           // bDrawAxis
    );
    
    // XZ 平面 (垂直圆 - 前后)
    DrawDebugCircle(
        World,
        AgentLocation,
        CircleRadius,
        24,
        DrawColor,
        false,
        -1.f,
        0,
        2.f,
        FVector(1, 0, 0),  // YAxis
        FVector(0, 0, 1),  // ZAxis
        false
    );
    
    // YZ 平面 (垂直圆 - 左右)
    DrawDebugCircle(
        World,
        AgentLocation,
        CircleRadius,
        24,
        DrawColor,
        false,
        -1.f,
        0,
        2.f,
        FVector(0, 1, 0),  // YAxis
        FVector(0, 0, 1),  // ZAxis
        false
    );
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
    FString ModeName = AMAPlayerController::MouseModeToString(PC->CurrentMouseMode);
    FString ModeText = FString::Printf(TEXT("Mode: %s (M)"), *ModeName);

    // 计算位置（右上角）
    float TextWidth = 150.f;
    float X = Canvas->SizeX - TextWidth - 20.f;
    float Y = 20.f;

    // 根据模式选择颜色
    FLinearColor ModeColor;
    switch (PC->CurrentMouseMode)
    {
        case EMAMouseMode::Select: ModeColor = FLinearColor::Green; break;
        case EMAMouseMode::Deployment: ModeColor = FLinearColor(0.2f, 0.6f, 1.f, 1.f); break;
        case EMAMouseMode::Modify: ModeColor = FLinearColor(1.f, 0.6f, 0.f, 1.f); break;  // 橙色
        case EMAMouseMode::Edit: ModeColor = FLinearColor(0.f, 0.5f, 1.f, 1.f); break;    // 蓝色
        default: ModeColor = FLinearColor::White; break;
    }

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

    // 绘制背景
    float BoxWidth = 450.f;
    float BoxHeight = 55.f;
    FLinearColor BgColor = bInDeploymentMode 
        ? FLinearColor(0.f, 0.1f, 0.3f, 0.8f)  // 部署模式：深蓝
        : FLinearColor(0.1f, 0.1f, 0.1f, 0.7f); // 非部署模式：深灰
    
    FCanvasTileItem BgItem(
        FVector2D(CenterX - BoxWidth / 2.f, Y - 5.f),
        FVector2D(BoxWidth, BoxHeight),
        BgColor
    );
    BgItem.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(BgItem);

    // 绘制文字
    FLinearColor InfoColor = bInDeploymentMode 
        ? FLinearColor(0.2f, 0.8f, 1.f, 1.f)   // 部署模式：亮蓝
        : FLinearColor(0.7f, 0.7f, 0.7f, 1.f); // 非部署模式：灰色
    
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
