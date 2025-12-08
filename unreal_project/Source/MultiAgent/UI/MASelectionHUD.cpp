// MASelectionHUD.cpp
// 选择框 HUD 实现

#include "MASelectionHUD.h"
#include "../Core/MASelectionManager.h"
#include "../Input/MAPlayerController.h"
#include "../Agent/Character/MACharacter.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"

AMASelectionHUD::AMASelectionHUD()
{
}

void AMASelectionHUD::DrawHUD()
{
    Super::DrawHUD();

    // 绘制框选矩形
    if (bIsBoxSelecting)
    {
        DrawSelectionBox();
    }

    // 绘制选中 Agent 的高亮
    DrawSelectedAgents();

    // 绘制编组信息
    DrawControlGroupInfo();

    // 绘制当前鼠标模式
    DrawMouseMode();
}

void AMASelectionHUD::DrawSelectionBox()
{
    if (!Canvas) return;

    float MinX = FMath::Min(BoxStart.X, BoxEnd.X);
    float MaxX = FMath::Max(BoxStart.X, BoxEnd.X);
    float MinY = FMath::Min(BoxStart.Y, BoxEnd.Y);
    float MaxY = FMath::Max(BoxStart.Y, BoxEnd.Y);

    float Width = MaxX - MinX;
    float Height = MaxY - MinY;

    // 绘制半透明填充
    FCanvasTileItem FillItem(
        FVector2D(MinX, MinY),
        FVector2D(Width, Height),
        BoxColor
    );
    FillItem.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(FillItem);

    // 绘制边框 (4 条线)
    float BorderThickness = 2.f;
    
    // 上边
    DrawLine(MinX, MinY, MaxX, MinY, BoxBorderColor, BorderThickness);
    // 下边
    DrawLine(MinX, MaxY, MaxX, MaxY, BoxBorderColor, BorderThickness);
    // 左边
    DrawLine(MinX, MinY, MinX, MaxY, BoxBorderColor, BorderThickness);
    // 右边
    DrawLine(MaxX, MinY, MaxX, MaxY, BoxBorderColor, BorderThickness);
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
    if (!Agent || !Canvas) return;

    APlayerController* PC = GetOwningPlayerController();
    if (!PC) return;

    // 获取 Agent 脚下位置的屏幕坐标
    FVector FootLocation = Agent->GetActorLocation();
    FootLocation.Z -= 90.f;  // 大约在脚下

    FVector2D ScreenPos;
    if (!PC->ProjectWorldLocationToScreen(FootLocation, ScreenPos))
    {
        return;  // 不在屏幕内
    }

    // 绘制圆圈 (用多边形近似)
    const int32 NumSegments = 24;
    const float AngleStep = 2.f * PI / NumSegments;

    for (int32 i = 0; i < NumSegments; i++)
    {
        float Angle1 = i * AngleStep;
        float Angle2 = (i + 1) * AngleStep;

        FVector2D P1(
            ScreenPos.X + FMath::Cos(Angle1) * Radius,
            ScreenPos.Y + FMath::Sin(Angle1) * Radius * 0.5f  // 椭圆，模拟透视
        );
        FVector2D P2(
            ScreenPos.X + FMath::Cos(Angle2) * Radius,
            ScreenPos.Y + FMath::Sin(Angle2) * Radius * 0.5f
        );

        DrawLine(P1.X, P1.Y, P2.X, P2.Y, Color, 2.f);
    }
}

void AMASelectionHUD::DrawMouseMode()
{
    if (!Canvas) return;

    AMAPlayerController* PC = Cast<AMAPlayerController>(GetOwningPlayerController());
    if (!PC) return;

    // 在屏幕右上角显示当前模式
    FString ModeName = AMAPlayerController::MouseModeToString(PC->CurrentMouseMode);
    FString ModeText = FString::Printf(TEXT("Mode: %s (M)"), *ModeName);

    // 计算位置（右上角）
    float TextWidth = 150.f;
    float X = Canvas->SizeX - TextWidth - 20.f;
    float Y = 20.f;

    // 根据模式选择颜色
    FLinearColor ModeColor = (PC->CurrentMouseMode == EMAMouseMode::Select) 
        ? FLinearColor::Green 
        : FLinearColor::Yellow;

    FCanvasTextItem TextItem(
        FVector2D(X, Y),
        FText::FromString(ModeText),
        GEngine->GetSmallFont(),
        ModeColor
    );
    Canvas->DrawItem(TextItem);
}
