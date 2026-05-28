// MAMiniMapWidget.cpp
// 小地图 Widget 实现

#include "MAMiniMapWidget.h"
#include "../../Core/MAUITheme.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

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
    ApplyFrameModel(Coordinator.BuildFrameModel(this, BuildViewportConfig()));
}

void UMAMiniMapWidget::UpdateCameraIndicator(FVector CameraLocation, FRotator CameraRotation)
{
    ApplyCameraIndicator(Coordinator.BuildCameraIndicator(BuildViewportConfig(), CameraLocation, CameraRotation));
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
        if (Coordinator.HandleMiniMapClick(this, BuildViewportConfig(), LocalPos))
        {
            return FReply::Handled();
        }
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
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

FMAMiniMapViewportConfig UMAMiniMapWidget::BuildViewportConfig() const
{
    FMAMiniMapViewportConfig Config;
    Config.MiniMapSize = MiniMapSize;
    Config.WorldBounds = WorldBounds;
    Config.WorldCenter = WorldCenter;
    Config.AgentIconSize = AgentIconSize;
    Config.CameraIconSize = CameraIconSize;
    Config.CameraFOVLength = CameraFOVLength;
    Config.CameraFOVAngle = CameraFOVAngle;
    return Config;
}

void UMAMiniMapWidget::ApplyFrameModel(const FMAMiniMapFrameModel& Model)
{
    if (!IconCanvas)
    {
        return;
    }

    IconCanvas->ClearChildren();
    for (const FMAMiniMapAgentMarker& Marker : Model.AgentMarkers)
    {
        UImage* Icon = NewObject<UImage>(this);
        if (!Icon)
        {
            continue;
        }

        Icon->SetColorAndOpacity(Marker.Color);
        Icon->SetBrushTintColor(FSlateColor(Marker.Color));
        IconCanvas->AddChild(Icon);

        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Icon->Slot))
        {
            Slot->SetSize(FVector2D(Marker.Size, Marker.Size));
            Slot->SetPosition(Marker.Position - FVector2D(Marker.Size / 2.0f, Marker.Size / 2.0f));
        }
    }

    ApplyCameraIndicator(Model.CameraIndicator);
}

void UMAMiniMapWidget::ApplyCameraIndicator(const FMAMiniMapCameraIndicatorModel& Model)
{
    if (!CameraIcon || !CameraFOVLeft || !CameraFOVRight)
    {
        return;
    }

    const ESlateVisibility Visibility = Model.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    CameraIcon->SetVisibility(Visibility);
    CameraFOVLeft->SetVisibility(Visibility);
    CameraFOVRight->SetVisibility(Visibility);

    if (!Model.bVisible)
    {
        return;
    }

    const FVector2D ScreenPos = Model.Position + FVector2D(20.0f, 20.0f);
    if (UCanvasPanelSlot* CamSlot = Cast<UCanvasPanelSlot>(CameraIcon->Slot))
    {
        CamSlot->SetSize(FVector2D(Model.IconSize, Model.IconSize));
        CamSlot->SetPosition(ScreenPos - FVector2D(Model.IconSize / 2.0f, Model.IconSize / 2.0f));
    }

    if (UCanvasPanelSlot* LeftSlot = Cast<UCanvasPanelSlot>(CameraFOVLeft->Slot))
    {
        LeftSlot->SetPosition(ScreenPos - FVector2D(1.0f, 0.0f));
        LeftSlot->SetSize(FVector2D(2.0f, Model.FOVLength));
        CameraFOVLeft->SetRenderTransformAngle(Model.LeftAngle);
        CameraFOVLeft->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));
    }

    if (UCanvasPanelSlot* RightSlot = Cast<UCanvasPanelSlot>(CameraFOVRight->Slot))
    {
        RightSlot->SetPosition(ScreenPos - FVector2D(1.0f, 0.0f));
        RightSlot->SetSize(FVector2D(2.0f, Model.FOVLength));
        CameraFOVRight->SetRenderTransformAngle(Model.RightAngle);
        CameraFOVRight->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));
    }
}
