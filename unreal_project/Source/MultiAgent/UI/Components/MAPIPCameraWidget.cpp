// MAPIPCameraWidget.cpp
// 单个画中画相机显示 Widget 实现

#include "MAPIPCameraWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"
#include "Brushes/SlateColorBrush.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAPIPWidget, Log, All);

void UMAPIPCameraWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UMAPIPCameraWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogMAPIPWidget, Warning, TEXT("NativeConstruct called, WidgetTree=%s"), 
        WidgetTree ? TEXT("Valid") : TEXT("NULL"));
    
    BuildUI();
}

void UMAPIPCameraWidget::BuildUI()
{
    // 清理现有内容
    if (RootCanvas)
    {
        return;  // 已构建
    }

    // 创建根 Canvas 并设置为 WidgetTree 的根
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    if (!RootCanvas)
    {
        UE_LOG(LogMAPIPWidget, Error, TEXT("Failed to create RootCanvas"));
        return;
    }
    
    WidgetTree->RootWidget = RootCanvas;

    //=========================================================================
    // 阴影层
    //=========================================================================
    
    ShadowImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ShadowImage"));
    if (ShadowImage)
    {
        RootCanvas->AddChild(ShadowImage);
        
        if (UCanvasPanelSlot* ShadowSlot = Cast<UCanvasPanelSlot>(ShadowImage->Slot))
        {
            ShadowSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
            ShadowSlot->SetPosition(FVector2D(6.f, 6.f));  // 阴影偏移
            ShadowSlot->SetSize(CurrentSize);
        }
        
        // 阴影颜色
        FLinearColor ShadowColor = FLinearColor(0.f, 0.f, 0.f, 0.4f);
        ShadowImage->SetColorAndOpacity(ShadowColor);
        
        UE_LOG(LogMAPIPWidget, Log, TEXT("Shadow image created"));
    }

    //=========================================================================
    // 主边框容器
    //=========================================================================
    
    MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBorder"));
    if (MainBorder)
    {
        RootCanvas->AddChild(MainBorder);
        
        if (UCanvasPanelSlot* BorderSlot = Cast<UCanvasPanelSlot>(MainBorder->Slot))
        {
            BorderSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
            BorderSlot->SetPosition(FVector2D(0.f, 0.f));
            BorderSlot->SetSize(CurrentSize);
        }
        
        // 边框样式
        MainBorder->SetBrushColor(DisplayConfig.BorderColor);
        MainBorder->SetPadding(FMargin(DisplayConfig.BorderThickness));
        
        UE_LOG(LogMAPIPWidget, Log, TEXT("Main border created"));
    }

    //=========================================================================
    // 内容叠加层
    //=========================================================================
    
    ContentOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ContentOverlay"));
    if (ContentOverlay && MainBorder)
    {
        MainBorder->AddChild(ContentOverlay);
    }

    //=========================================================================
    // 相机画面
    //=========================================================================
    
    CameraImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CameraImage"));
    if (CameraImage && ContentOverlay)
    {
        ContentOverlay->AddChild(CameraImage);
        
        if (UOverlaySlot* ImageSlot = Cast<UOverlaySlot>(CameraImage->Slot))
        {
            ImageSlot->SetHorizontalAlignment(HAlign_Fill);
            ImageSlot->SetVerticalAlignment(VAlign_Fill);
        }
        
        // 默认黑色背景
        CameraImage->SetColorAndOpacity(FLinearColor::White);
        
        UE_LOG(LogMAPIPWidget, Log, TEXT("Camera image created"));
    }

    //=========================================================================
    // 标题栏
    //=========================================================================
    
    TitleBarBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleBarBorder"));
    if (TitleBarBorder && ContentOverlay)
    {
        ContentOverlay->AddChild(TitleBarBorder);
        
        if (UOverlaySlot* TitleSlot = Cast<UOverlaySlot>(TitleBarBorder->Slot))
        {
            TitleSlot->SetHorizontalAlignment(HAlign_Fill);
            TitleSlot->SetVerticalAlignment(VAlign_Top);
        }
        
        // 标题栏背景（半透明黑色）
        TitleBarBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
        TitleBarBorder->SetPadding(FMargin(8.f, 4.f));
        
        // 标题文本
        TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
        if (TitleText)
        {
            TitleBarBorder->AddChild(TitleText);
            TitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
            
            FSlateFontInfo FontInfo = TitleText->GetFont();
            FontInfo.Size = 12;
            TitleText->SetFont(FontInfo);
        }
        
        // 默认隐藏标题栏
        TitleBarBorder->SetVisibility(ESlateVisibility::Collapsed);
        
        UE_LOG(LogMAPIPWidget, Log, TEXT("Title bar created"));
    }
    
    UE_LOG(LogMAPIPWidget, Log, TEXT("BuildUI completed"));
}

void UMAPIPCameraWidget::SetRenderTarget(UTextureRenderTarget2D* RenderTarget)
{
    UE_LOG(LogMAPIPWidget, Warning, TEXT("SetRenderTarget called, RenderTarget=%s, CameraImage=%s"),
        RenderTarget ? TEXT("Valid") : TEXT("NULL"),
        CameraImage ? TEXT("Valid") : TEXT("NULL"));
    
    if (!CameraImage)
    {
        UE_LOG(LogMAPIPWidget, Error, TEXT("SetRenderTarget: CameraImage is NULL!"));
        return;
    }
    
    if (RenderTarget)
    {
        // 从渲染目标创建 Brush
        FSlateBrush Brush;
        Brush.SetResourceObject(RenderTarget);
        Brush.ImageSize = FVector2D(RenderTarget->SizeX, RenderTarget->SizeY);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        
        CameraImage->SetBrush(Brush);
        UE_LOG(LogMAPIPWidget, Log, TEXT("SetRenderTarget: Brush set, size=%dx%d"), 
            RenderTarget->SizeX, RenderTarget->SizeY);
    }
    else
    {
        // 清除，显示黑色
        CameraImage->SetBrushFromTexture(nullptr);
        CameraImage->SetColorAndOpacity(FLinearColor::Black);
    }
}

void UMAPIPCameraWidget::SetDisplayConfig(const FMAPIPDisplayConfig& Config)
{
    DisplayConfig = Config;
    
    // 更新大小
    SetSize(Config.Size);
    
    // 更新标题
    SetTitle(Config.Title);
    
    // 更新阴影
    UpdateShadow();
    
    // 更新边框
    UpdateBorder();
}

void UMAPIPCameraWidget::SetTitle(const FString& Title)
{
    if (!TitleText || !TitleBarBorder)
    {
        return;
    }
    
    if (Title.IsEmpty())
    {
        TitleBarBorder->SetVisibility(ESlateVisibility::Collapsed);
    }
    else
    {
        TitleText->SetText(FText::FromString(Title));
        TitleBarBorder->SetVisibility(ESlateVisibility::Visible);
    }
}

void UMAPIPCameraWidget::SetSize(const FVector2D& Size)
{
    CurrentSize = Size;
    
    // 更新阴影大小
    if (ShadowImage)
    {
        if (UCanvasPanelSlot* ShadowSlot = Cast<UCanvasPanelSlot>(ShadowImage->Slot))
        {
            ShadowSlot->SetSize(Size);
        }
    }
    
    // 更新主边框大小
    if (MainBorder)
    {
        if (UCanvasPanelSlot* BorderSlot = Cast<UCanvasPanelSlot>(MainBorder->Slot))
        {
            BorderSlot->SetSize(Size);
        }
    }
}

void UMAPIPCameraWidget::UpdateShadow()
{
    if (!ShadowImage)
    {
        return;
    }
    
    if (DisplayConfig.bShowShadow)
    {
        ShadowImage->SetVisibility(ESlateVisibility::Visible);
        
        // 阴影偏移
        if (UCanvasPanelSlot* ShadowSlot = Cast<UCanvasPanelSlot>(ShadowImage->Slot))
        {
            ShadowSlot->SetPosition(FVector2D(6.f, 6.f));
        }
    }
    else
    {
        ShadowImage->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMAPIPCameraWidget::UpdateBorder()
{
    if (!MainBorder)
    {
        return;
    }
    
    if (DisplayConfig.bShowBorder)
    {
        MainBorder->SetBrushColor(DisplayConfig.BorderColor);
        MainBorder->SetPadding(FMargin(DisplayConfig.BorderThickness));
    }
    else
    {
        MainBorder->SetBrushColor(FLinearColor::Transparent);
        MainBorder->SetPadding(FMargin(0.f));
    }
}

void UMAPIPCameraWidget::UpdateTitleBar()
{
    // 已在 SetTitle 中处理
}
