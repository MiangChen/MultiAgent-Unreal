// MAPIPCameraContainerWidget.cpp
// 画中画相机容器 Widget 实现

#include "MAPIPCameraContainerWidget.h"
#include "MAPIPCameraWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAPIPContainer, Log, All);

void UMAPIPCameraContainerWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMAPIPCameraContainerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogMAPIPContainer, Log, TEXT("NativeConstruct called"));
}

TSharedRef<SWidget> UMAPIPCameraContainerWidget::RebuildWidget()
{
    UE_LOG(LogMAPIPContainer, Warning, TEXT("RebuildWidget called"));
    
    // 创建根 Canvas
    RootCanvas = NewObject<UCanvasPanel>(this);

    // 设置容器为不阻挡输入
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    
    UE_LOG(LogMAPIPContainer, Log, TEXT("RebuildWidget completed"));
    
    return RootCanvas->TakeWidget();
}

void UMAPIPCameraContainerWidget::BuildUI()
{
    // 已在 RebuildWidget 中处理
}

int32 UMAPIPCameraContainerWidget::AddPIPCamera(UTextureRenderTarget2D* RenderTarget, const FMAPIPDisplayConfig& DisplayConfig)
{
    UE_LOG(LogMAPIPContainer, Warning, TEXT("AddPIPCamera called, RootCanvas=%s, RenderTarget=%s"),
        RootCanvas ? TEXT("Valid") : TEXT("NULL"),
        RenderTarget ? TEXT("Valid") : TEXT("NULL"));
    
    if (!RootCanvas)
    {
        UE_LOG(LogMAPIPContainer, Error, TEXT("AddPIPCamera: RootCanvas is null, calling BuildUI"));
        BuildUI();
        if (!RootCanvas)
        {
            UE_LOG(LogMAPIPContainer, Error, TEXT("AddPIPCamera: RootCanvas still null after BuildUI"));
            return -1;
        }
    }

    // 创建新的 PIP Widget
    UMAPIPCameraWidget* PIPWidget = CreateWidget<UMAPIPCameraWidget>(GetOwningPlayer(), UMAPIPCameraWidget::StaticClass());
    if (!PIPWidget)
    {
        UE_LOG(LogMAPIPContainer, Error, TEXT("AddPIPCamera: Failed to create PIPWidget"));
        return -1;
    }
    
    UE_LOG(LogMAPIPContainer, Log, TEXT("AddPIPCamera: PIPWidget created successfully"));

    // 添加到 Canvas
    RootCanvas->AddChild(PIPWidget);

    // 计算屏幕位置
    FVector2D ScreenPos = CalculateScreenPosition(DisplayConfig.ScreenPosition, DisplayConfig.Size);

    // 设置位置和大小
    if (UCanvasPanelSlot* WidgetSlot = Cast<UCanvasPanelSlot>(PIPWidget->Slot))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
        WidgetSlot->SetPosition(ScreenPos);
        WidgetSlot->SetSize(DisplayConfig.Size + FVector2D(12.f, 12.f));  // 额外空间给阴影
        WidgetSlot->SetAutoSize(false);
        UE_LOG(LogMAPIPContainer, Log, TEXT("AddPIPCamera: Slot configured, pos=(%.0f, %.0f), size=(%.0f, %.0f)"),
            ScreenPos.X, ScreenPos.Y, DisplayConfig.Size.X + 12.f, DisplayConfig.Size.Y + 12.f);
    }
    else
    {
        UE_LOG(LogMAPIPContainer, Error, TEXT("AddPIPCamera: Failed to get CanvasPanelSlot"));
    }

    // 设置显示配置
    PIPWidget->SetDisplayConfig(DisplayConfig);

    // 设置渲染目标
    PIPWidget->SetRenderTarget(RenderTarget);

    // 添加到列表
    int32 Index = PIPWidgets.Add(PIPWidget);

    UE_LOG(LogMAPIPContainer, Warning, TEXT("AddPIPCamera: Added PIP camera at index %d, position (%.0f, %.0f)"),
        Index, ScreenPos.X, ScreenPos.Y);

    return Index;
}

void UMAPIPCameraContainerWidget::RemovePIPCamera(int32 Index)
{
    if (!PIPWidgets.IsValidIndex(Index))
    {
        UE_LOG(LogMAPIPContainer, Warning, TEXT("RemovePIPCamera: Invalid index %d"), Index);
        return;
    }

    UMAPIPCameraWidget* PIPWidget = PIPWidgets[Index];
    if (PIPWidget)
    {
        PIPWidget->RemoveFromParent();
    }

    PIPWidgets.RemoveAt(Index);

    UE_LOG(LogMAPIPContainer, Log, TEXT("RemovePIPCamera: Removed PIP camera at index %d"), Index);
}

void UMAPIPCameraContainerWidget::UpdatePIPCameraTexture(int32 Index, UTextureRenderTarget2D* RenderTarget)
{
    if (!PIPWidgets.IsValidIndex(Index))
    {
        UE_LOG(LogMAPIPContainer, Warning, TEXT("UpdatePIPCameraTexture: Invalid index %d"), Index);
        return;
    }

    UMAPIPCameraWidget* PIPWidget = PIPWidgets[Index];
    if (PIPWidget)
    {
        PIPWidget->SetRenderTarget(RenderTarget);
    }
}

void UMAPIPCameraContainerWidget::UpdatePIPCameraDisplay(int32 Index, const FMAPIPDisplayConfig& DisplayConfig)
{
    if (!PIPWidgets.IsValidIndex(Index))
    {
        UE_LOG(LogMAPIPContainer, Warning, TEXT("UpdatePIPCameraDisplay: Invalid index %d"), Index);
        return;
    }

    UMAPIPCameraWidget* PIPWidget = PIPWidgets[Index];
    if (!PIPWidget)
    {
        return;
    }

    // 更新显示配置
    PIPWidget->SetDisplayConfig(DisplayConfig);

    // 更新位置
    FVector2D ScreenPos = CalculateScreenPosition(DisplayConfig.ScreenPosition, DisplayConfig.Size);
    if (UCanvasPanelSlot* WidgetSlot = Cast<UCanvasPanelSlot>(PIPWidget->Slot))
    {
        WidgetSlot->SetPosition(ScreenPos);
        WidgetSlot->SetSize(DisplayConfig.Size + FVector2D(12.f, 12.f));
    }
}

void UMAPIPCameraContainerWidget::ClearAllPIPCameras()
{
    for (UMAPIPCameraWidget* PIPWidget : PIPWidgets)
    {
        if (PIPWidget)
        {
            PIPWidget->RemoveFromParent();
        }
    }

    PIPWidgets.Empty();

    UE_LOG(LogMAPIPContainer, Log, TEXT("ClearAllPIPCameras: Cleared all PIP cameras"));
}

FVector2D UMAPIPCameraContainerWidget::CalculateScreenPosition(const FVector2D& NormalizedPosition, const FVector2D& WidgetSize) const
{
    FVector2D ViewportSize = GetViewportSize();
    
    // 归一化坐标转换为像素坐标
    // NormalizedPosition 表示 Widget 中心点的位置
    FVector2D CenterPos = FVector2D(
        NormalizedPosition.X * ViewportSize.X,
        NormalizedPosition.Y * ViewportSize.Y
    );
    
    // 转换为左上角位置
    FVector2D TopLeftPos = CenterPos - WidgetSize * 0.5f;
    
    // 确保不超出屏幕边界
    TopLeftPos.X = FMath::Clamp(TopLeftPos.X, 0.f, ViewportSize.X - WidgetSize.X - 12.f);
    TopLeftPos.Y = FMath::Clamp(TopLeftPos.Y, 0.f, ViewportSize.Y - WidgetSize.Y - 12.f);
    
    return TopLeftPos;
}

FVector2D UMAPIPCameraContainerWidget::GetViewportSize() const
{
    if (GEngine && GEngine->GameViewport)
    {
        FVector2D ViewportSize;
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        return ViewportSize;
    }
    
    // 默认值
    return FVector2D(1920.f, 1080.f);
}
