// MAMiniMapWidget.cpp
// 小地图 Widget 实现

#include "MAMiniMapWidget.h"
#include "../Core/MAUITheme.h"
#include "../../Core/Manager/MAAgentManager.h"
#include "../../Core/Manager/MASelectionManager.h"
#include "../../Agent/Character/MACharacter.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

//=============================================================================
// 主题辅助函数
//=============================================================================

namespace
{
    /** 获取主题或创建默认主题 */
    UMAUITheme* GetOrCreateDefaultTheme()
    {
        static UMAUITheme* DefaultTheme = nullptr;
        if (!DefaultTheme)
        {
            DefaultTheme = NewObject<UMAUITheme>();
            DefaultTheme->AddToRoot(); // 防止被 GC
        }
        return DefaultTheme;
    }
}

void UMAMiniMapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 纯 C++ Widget 需要在 TakeWidget() 时构建 Slate
    // 这里只做初始化标记
    UE_LOG(LogTemp, Log, TEXT("[MiniMap] Widget NativeConstruct, size: %.0f"), MiniMapSize);
}

TSharedRef<SWidget> UMAMiniMapWidget::RebuildWidget()
{
    // 获取主题
    UMAUITheme* CurrentTheme = Theme ? Theme : GetOrCreateDefaultTheme();
    
    // 创建根 Canvas
    RootCanvas = NewObject<UCanvasPanel>(this);
    
    // 创建背景图片
    BackgroundImage = NewObject<UImage>(this);
    BackgroundImage->SetDesiredSizeOverride(FVector2D(MiniMapSize, MiniMapSize));
    BackgroundImage->SetColorAndOpacity(CurrentTheme->MiniMapBackgroundColor);

    // 创建图标容器
    IconCanvas = NewObject<UCanvasPanel>(this);

    // 创建相机视野左边线 (使用主题相机颜色)
    FLinearColor CameraColor = CurrentTheme->MiniMapCameraColor;
    FLinearColor CameraColorSemiTransparent = FLinearColor(CameraColor.R, CameraColor.G, CameraColor.B, 0.8f);
    
    CameraFOVLeft = NewObject<UImage>(this);
    CameraFOVLeft->SetColorAndOpacity(CameraColorSemiTransparent);
    CameraFOVLeft->SetDesiredSizeOverride(FVector2D(2.f, CameraFOVLength));

    // 创建相机视野右边线
    CameraFOVRight = NewObject<UImage>(this);
    CameraFOVRight->SetColorAndOpacity(CameraColorSemiTransparent);
    CameraFOVRight->SetDesiredSizeOverride(FVector2D(2.f, CameraFOVLength));

    // 创建相机图标
    CameraIcon = NewObject<UImage>(this);
    CameraIcon->SetColorAndOpacity(CameraColor);
    CameraIcon->SetDesiredSizeOverride(FVector2D(CameraIconSize, CameraIconSize));

    // 添加到根 Canvas (顺序：背景 -> 图标 -> 视野线 -> 相机)
    RootCanvas->AddChild(BackgroundImage);
    RootCanvas->AddChild(IconCanvas);
    RootCanvas->AddChild(CameraFOVLeft);
    RootCanvas->AddChild(CameraFOVRight);
    RootCanvas->AddChild(CameraIcon);

    // 设置背景位置 (左上角)
    if (UCanvasPanelSlot* BgSlot = Cast<UCanvasPanelSlot>(BackgroundImage->Slot))
    {
        BgSlot->SetPosition(FVector2D(20.f, 20.f));
        BgSlot->SetSize(FVector2D(MiniMapSize, MiniMapSize));
        BgSlot->SetAutoSize(false);
    }

    // 设置图标容器位置
    if (UCanvasPanelSlot* IconSlot = Cast<UCanvasPanelSlot>(IconCanvas->Slot))
    {
        IconSlot->SetPosition(FVector2D(20.f, 20.f));
        IconSlot->SetSize(FVector2D(MiniMapSize, MiniMapSize));
        IconSlot->SetAutoSize(false);
    }

    // 设置相机图标位置 (初始在中心)
    if (UCanvasPanelSlot* CamSlot = Cast<UCanvasPanelSlot>(CameraIcon->Slot))
    {
        CamSlot->SetPosition(FVector2D(20.f + MiniMapSize / 2.f - CameraIconSize / 2.f, 
                                        20.f + MiniMapSize / 2.f - CameraIconSize / 2.f));
        CamSlot->SetSize(FVector2D(CameraIconSize, CameraIconSize));
        CamSlot->SetAutoSize(false);
    }

    // 设置视野线初始位置
    if (UCanvasPanelSlot* LeftSlot = Cast<UCanvasPanelSlot>(CameraFOVLeft->Slot))
    {
        LeftSlot->SetSize(FVector2D(2.f, CameraFOVLength));
        LeftSlot->SetAutoSize(false);
    }
    if (UCanvasPanelSlot* RightSlot = Cast<UCanvasPanelSlot>(CameraFOVRight->Slot))
    {
        RightSlot->SetSize(FVector2D(2.f, CameraFOVLength));
        RightSlot->SetAutoSize(false);
    }

    UE_LOG(LogTemp, Log, TEXT("[MiniMap] Widget rebuilt with FOV indicator"));

    return RootCanvas->TakeWidget();
}

void UMAMiniMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UpdateTimer += InDeltaTime;
    if (UpdateTimer >= UpdateInterval)
    {
        UpdateTimer = 0.f;
        UpdateAgentPositions();

        // 更新相机位置
        if (APlayerController* PC = GetOwningPlayer())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                UpdateCameraIndicator(Pawn->GetActorLocation(), Pawn->GetActorRotation());
            }
        }
    }
}

void UMAMiniMapWidget::InitializeMiniMap(UTextureRenderTarget2D* InRenderTarget, FVector2D InWorldBounds)
{
    RenderTarget = InRenderTarget;
    WorldBounds = InWorldBounds;

    if (BackgroundImage && RenderTarget)
    {
        // 创建动态材质显示 RenderTarget
        UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(
            BackgroundImage->GetBrush().GetResourceObject() ? 
                Cast<UMaterialInterface>(BackgroundImage->GetBrush().GetResourceObject()) : nullptr,
            this
        );
        
        if (DynMaterial)
        {
            DynMaterial->SetTextureParameterValue(TEXT("MiniMapTexture"), RenderTarget);
            BackgroundImage->SetBrushFromMaterial(DynMaterial);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MiniMap] Initialized with bounds (%.0f, %.0f)"), WorldBounds.X, WorldBounds.Y);
}

void UMAMiniMapWidget::UpdateAgentPositions()
{
    if (!IconCanvas) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UMAAgentManager* AgentManager = World->GetSubsystem<UMAAgentManager>();
    UMASelectionManager* SelectionManager = World->GetSubsystem<UMASelectionManager>();
    if (!AgentManager) return;

    // 清除旧图标 (简单实现：每帧重绘)
    IconCanvas->ClearChildren();

    // 获取选中的 Agent
    TArray<AMACharacter*> SelectedAgents;
    if (SelectionManager)
    {
        SelectedAgents = SelectionManager->GetSelectedAgents();
    }

    // 绘制所有 Agent
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (!Agent) continue;

        FVector2D MiniMapPos = WorldToMiniMap(Agent->GetActorLocation());

        // 创建图标
        UImage* Icon = NewObject<UImage>(this);
        if (!Icon) continue;

        // 获取 Agent 颜色 (使用主题)
        bool bIsSelected = SelectedAgents.Contains(Agent);
        FLinearColor IconColor = GetAgentColor(Agent, bIsSelected);

        // 设置图标颜色
        Icon->SetColorAndOpacity(IconColor);
        Icon->SetBrushTintColor(FSlateColor(IconColor));

        // 添加到 Canvas
        IconCanvas->AddChild(Icon);

        // 设置位置
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Icon->Slot))
        {
            Slot->SetSize(FVector2D(AgentIconSize, AgentIconSize));
            Slot->SetPosition(MiniMapPos - FVector2D(AgentIconSize / 2.f, AgentIconSize / 2.f));
        }
    }
}

FLinearColor UMAMiniMapWidget::GetAgentColor(AMACharacter* Agent, bool bIsSelected) const
{
    // 获取主题
    UMAUITheme* CurrentTheme = Theme ? Theme : GetOrCreateDefaultTheme();
    
    // 选中的 Agent 用选中颜色
    if (bIsSelected)
    {
        return CurrentTheme->AgentSelectedColor;
    }
    
    // 根据类型返回颜色
    switch (Agent->AgentType)
    {
        case EMAAgentType::Humanoid:
            return CurrentTheme->AgentHumanoidColor;
        case EMAAgentType::UAV:
        case EMAAgentType::FixedWingUAV:
            return CurrentTheme->AgentUAVColor;
        case EMAAgentType::UGV:
            return CurrentTheme->AgentUGVColor;
        case EMAAgentType::Quadruped:
            return CurrentTheme->AgentQuadrupedColor;
        default:
            return FLinearColor::White;
    }
}

void UMAMiniMapWidget::UpdateCameraIndicator(FVector CameraLocation, FRotator CameraRotation)
{
    if (!CameraIcon || !CameraFOVLeft || !CameraFOVRight) return;

    // 计算相机在小地图上的位置
    FVector2D MiniMapPos = WorldToMiniMap(CameraLocation);
    
    // 加上小地图的偏移 (20px 边距)
    FVector2D ScreenPos = MiniMapPos + FVector2D(20.f, 20.f);

    // 更新相机图标位置 (红色圆点)
    if (UCanvasPanelSlot* CamSlot = Cast<UCanvasPanelSlot>(CameraIcon->Slot))
    {
        CamSlot->SetPosition(ScreenPos - FVector2D(CameraIconSize / 2.f, CameraIconSize / 2.f));
    }

    // 计算视野角度
    // 左边线 = Yaw - FOV/2, 右边线 = Yaw + FOV/2
    float HalfFOV = CameraFOVAngle / 2.f;
    float LeftAngle = CameraRotation.Yaw - HalfFOV;
    float RightAngle = CameraRotation.Yaw + HalfFOV;

    // 更新左边视野线
    if (UCanvasPanelSlot* LeftSlot = Cast<UCanvasPanelSlot>(CameraFOVLeft->Slot))
    {
        LeftSlot->SetPosition(ScreenPos - FVector2D(1.f, 0.f));
        CameraFOVLeft->SetRenderTransformAngle(LeftAngle - 90.f);
        CameraFOVLeft->SetRenderTransformPivot(FVector2D(0.5f, 0.f));
    }

    // 更新右边视野线
    if (UCanvasPanelSlot* RightSlot = Cast<UCanvasPanelSlot>(CameraFOVRight->Slot))
    {
        RightSlot->SetPosition(ScreenPos - FVector2D(1.f, 0.f));
        CameraFOVRight->SetRenderTransformAngle(RightAngle - 90.f);
        CameraFOVRight->SetRenderTransformPivot(FVector2D(0.5f, 0.f));
    }
}

FVector2D UMAMiniMapWidget::WorldToMiniMap(FVector WorldLocation) const
{
    // 将世界坐标转换为小地图坐标 (0 ~ MiniMapSize)
    float X = (WorldLocation.X - WorldCenter.X + WorldBounds.X / 2.f) / WorldBounds.X * MiniMapSize;
    float Y = (WorldLocation.Y - WorldCenter.Y + WorldBounds.Y / 2.f) / WorldBounds.Y * MiniMapSize;

    // 限制在小地图范围内
    X = FMath::Clamp(X, 0.f, MiniMapSize);
    Y = FMath::Clamp(Y, 0.f, MiniMapSize);

    return FVector2D(X, Y);
}

FVector UMAMiniMapWidget::MiniMapToWorld(FVector2D MiniMapLocation) const
{
    // 将小地图坐标转换为世界坐标
    float X = (MiniMapLocation.X / MiniMapSize) * WorldBounds.X - WorldBounds.X / 2.f + WorldCenter.X;
    float Y = (MiniMapLocation.Y / MiniMapSize) * WorldBounds.Y - WorldBounds.Y / 2.f + WorldCenter.Y;

    return FVector(X, Y, 0.f);
}

FReply UMAMiniMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // 获取点击位置
        FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

        // 检查是否在圆形范围内
        FVector2D Center(MiniMapSize / 2.f, MiniMapSize / 2.f);
        float Distance = FVector2D::Distance(LocalPos, Center);

        if (Distance <= MiniMapSize / 2.f)
        {
            // 转换为世界坐标
            FVector WorldPos = MiniMapToWorld(LocalPos);

            // 移动相机到该位置
            if (APlayerController* PC = GetOwningPlayer())
            {
                if (APawn* Pawn = PC->GetPawn())
                {
                    FVector NewLocation = Pawn->GetActorLocation();
                    NewLocation.X = WorldPos.X;
                    NewLocation.Y = WorldPos.Y;
                    Pawn->SetActorLocation(NewLocation);

                    UE_LOG(LogTemp, Log, TEXT("[MiniMap] Camera moved to (%.0f, %.0f)"), WorldPos.X, WorldPos.Y);
                }
            }

            return FReply::Handled();
        }
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UMAMiniMapWidget::DrawAgentIcon(AMACharacter* Agent)
{
    // 由 UpdateAgentPositions 统一处理
}

void UMAMiniMapWidget::ApplyTheme(UMAUITheme* InTheme)
{
    Theme = InTheme;
    
    if (!Theme)
    {
        return;
    }
    
    // 更新背景颜色
    if (BackgroundImage)
    {
        BackgroundImage->SetColorAndOpacity(Theme->MiniMapBackgroundColor);
    }
    
    // 更新相机指示器颜色
    FLinearColor CameraColor = Theme->MiniMapCameraColor;
    FLinearColor CameraColorSemiTransparent = FLinearColor(CameraColor.R, CameraColor.G, CameraColor.B, 0.8f);
    
    if (CameraIcon)
    {
        CameraIcon->SetColorAndOpacity(CameraColor);
    }
    
    if (CameraFOVLeft)
    {
        CameraFOVLeft->SetColorAndOpacity(CameraColorSemiTransparent);
    }
    
    if (CameraFOVRight)
    {
        CameraFOVRight->SetColorAndOpacity(CameraColorSemiTransparent);
    }
    
    UE_LOG(LogTemp, Log, TEXT("[MiniMap] Theme applied"));
}
