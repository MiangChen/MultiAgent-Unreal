// MAFrostedGlassUtils.cpp
// 毛玻璃效果工具类实现

#include "MAFrostedGlassUtils.h"
#include "MARoundedBorderUtils.h"
#include "MAUITheme.h"
#include "Components/Border.h"
#include "Components/BackgroundBlur.h"
#include "Components/CanvasPanelSlot.h"
#include "Styling/SlateTypes.h"

FMAFrostedGlassResult MAFrostedGlassUtils::CreateFrostedGlassContainer(
    UWidgetTree* WidgetTree,
    UCanvasPanel* ParentCanvas,
    const FAnchors& Anchors,
    const FMargin& Offsets,
    const FMAFrostedGlassConfig& Config,
    const FString& NamePrefix)
{
    FMAFrostedGlassResult Result;
    
    if (!WidgetTree || !ParentCanvas)
    {
        return Result;
    }
    
    // 1. 创建阴影层 (如果启用)
    if (Config.bEnableShadow)
    {
        Result.ShadowBorder = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), 
            *FString::Printf(TEXT("%s_Shadow"), *NamePrefix)
        );
        
        FLinearColor ShadowColor(0.0f, 0.0f, 0.0f, Config.ShadowOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(Result.ShadowBorder, ShadowColor, Config.CornerRadius);
        Result.ShadowBorder->SetPadding(FMargin(0.0f));
        
        UCanvasPanelSlot* ShadowSlot = ParentCanvas->AddChildToCanvas(Result.ShadowBorder);
        ShadowSlot->SetAnchors(Anchors);
        // 阴影偏移：向右下偏移
        ShadowSlot->SetOffsets(FMargin(
            Offsets.Left + Config.ShadowOffset,
            Offsets.Top + Config.ShadowOffset,
            Offsets.Right - Config.ShadowOffset,
            Offsets.Bottom - Config.ShadowOffset
        ));
    }
    
    // 2. 创建玻璃边框层 (如果启用)
    if (Config.bEnableGlassBorder)
    {
        Result.GlassBorder = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            *FString::Printf(TEXT("%s_GlassBorder"), *NamePrefix)
        );
        
        FLinearColor GlassBorderColor(1.0f, 1.0f, 1.0f, Config.GlassBorderOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(Result.GlassBorder, GlassBorderColor, Config.CornerRadius);
        // 增加 padding 以确保 BackgroundBlur 的矩形边缘被圆角边框覆盖
        // padding 需要足够大以覆盖圆角区域
        float EffectivePadding = FMath::Max(Config.GlassBorderThickness, Config.CornerRadius * 0.3f);
        Result.GlassBorder->SetPadding(FMargin(EffectivePadding));
        // 启用裁剪，确保内容不会超出圆角边框
        Result.GlassBorder->SetClipping(EWidgetClipping::ClipToBoundsAlways);
        
        UCanvasPanelSlot* GlassSlot = ParentCanvas->AddChildToCanvas(Result.GlassBorder);
        GlassSlot->SetAnchors(Anchors);
        GlassSlot->SetOffsets(Offsets);
    }
    
    // 3. 创建模糊层
    Result.BlurBackground = WidgetTree->ConstructWidget<UBackgroundBlur>(
        UBackgroundBlur::StaticClass(),
        *FString::Printf(TEXT("%s_Blur"), *NamePrefix)
    );
    Result.BlurBackground->SetBlurStrength(Config.BlurStrength);
    Result.BlurBackground->SetApplyAlphaToBlur(true);
    // 设置低质量回退画刷为透明，避免黑色背景
    FSlateBrush TransparentFallbackBrush;
    TransparentFallbackBrush.TintColor = FSlateColor(FLinearColor::Transparent);
    Result.BlurBackground->SetLowQualityFallbackBrush(TransparentFallbackBrush);
    // 增加模糊层的 padding，使其边缘更靠内，避免超出圆角边框
    float BlurPadding = Config.ContentPadding + Config.CornerRadius * 0.15f;
    Result.BlurBackground->SetPadding(FMargin(BlurPadding));
    
    // 将模糊层添加到玻璃边框内，或直接添加到 Canvas
    if (Result.GlassBorder)
    {
        Result.GlassBorder->AddChild(Result.BlurBackground);
    }
    else
    {
        UCanvasPanelSlot* BlurSlot = ParentCanvas->AddChildToCanvas(Result.BlurBackground);
        BlurSlot->SetAnchors(Anchors);
        BlurSlot->SetOffsets(Offsets);
    }
    
    // 4. 创建光泽层 (如果启用)
    if (Config.bEnableGloss)
    {
        Result.GlossOverlay = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            *FString::Printf(TEXT("%s_Gloss"), *NamePrefix)
        );
        
        FLinearColor GlossColor(1.0f, 1.0f, 1.0f, Config.GlossOpacity);
        // 光泽层圆角稍小，适应内部
        MARoundedBorderUtils::ApplyRoundedCorners(Result.GlossOverlay, GlossColor, Config.CornerRadius - 2.0f);
        Result.GlossOverlay->SetPadding(FMargin(0.0f));
        
        Result.BlurBackground->AddChild(Result.GlossOverlay);
        
        // 内容容器就是光泽层
        Result.ContentContainer = Result.GlossOverlay;
    }
    else
    {
        // 没有光泽层时，创建一个透明的内容容器
        Result.ContentContainer = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            *FString::Printf(TEXT("%s_Content"), *NamePrefix)
        );
        Result.ContentContainer->SetBrushColor(FLinearColor::Transparent);
        Result.ContentContainer->SetPadding(FMargin(0.0f));
        
        Result.BlurBackground->AddChild(Result.ContentContainer);
    }
    
    return Result;
}

FMAFrostedGlassResult MAFrostedGlassUtils::CreateFromTheme(
    UWidgetTree* WidgetTree,
    UCanvasPanel* ParentCanvas,
    const UMAUITheme* Theme,
    const FAnchors& Anchors,
    const FMargin& Offsets,
    const FString& NamePrefix)
{
    FMAFrostedGlassConfig Config = GetConfigFromTheme(Theme);
    return CreateFrostedGlassContainer(WidgetTree, ParentCanvas, Anchors, Offsets, Config, NamePrefix);
}

FMAFrostedGlassConfig MAFrostedGlassUtils::GetConfigFromTheme(const UMAUITheme* Theme)
{
    FMAFrostedGlassConfig Config;
    
    if (Theme)
    {
        // 从主题获取毛玻璃相关参数
        Config.BlurStrength = Theme->FrostedGlassBlurStrength;
        Config.CornerRadius = Theme->FrostedGlassCornerRadius;
        Config.ShadowOffset = Theme->FrostedGlassShadowOffset;
        Config.ShadowOpacity = Theme->FrostedGlassShadowOpacity;
        Config.GlassBorderOpacity = Theme->FrostedGlassBorderOpacity;
        Config.GlassBorderThickness = Theme->FrostedGlassBorderThickness;
        Config.GlossOpacity = Theme->FrostedGlassGlossOpacity;
        Config.ContentPadding = Theme->FrostedGlassContentPadding;
        Config.bEnableShadow = Theme->bFrostedGlassEnableShadow;
        Config.bEnableGlassBorder = Theme->bFrostedGlassEnableBorder;
        Config.bEnableGloss = Theme->bFrostedGlassEnableGloss;
    }
    
    return Config;
}


FMAFrostedGlassResult MAFrostedGlassUtils::CreateFixedSizePanel(
    UWidgetTree* WidgetTree,
    UCanvasPanel* ParentCanvas,
    const FVector2D& Position,
    const FVector2D& Size,
    const FVector2D& Alignment,
    const FAnchors& AnchorPoint,
    const FMAFrostedGlassConfig& Config,
    const FString& NamePrefix)
{
    FMAFrostedGlassResult Result;
    
    if (!WidgetTree || !ParentCanvas)
    {
        return Result;
    }
    
    // 1. 创建阴影层 (如果启用)
    if (Config.bEnableShadow)
    {
        Result.ShadowBorder = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), 
            *FString::Printf(TEXT("%s_Shadow"), *NamePrefix)
        );
        
        FLinearColor ShadowColor(0.0f, 0.0f, 0.0f, Config.ShadowOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(Result.ShadowBorder, ShadowColor, Config.CornerRadius);
        Result.ShadowBorder->SetPadding(FMargin(0.0f));
        
        UCanvasPanelSlot* ShadowSlot = ParentCanvas->AddChildToCanvas(Result.ShadowBorder);
        ShadowSlot->SetAnchors(AnchorPoint);
        ShadowSlot->SetAlignment(Alignment);
        // 阴影偏移
        ShadowSlot->SetPosition(FVector2D(Position.X + Config.ShadowOffset, Position.Y + Config.ShadowOffset));
        ShadowSlot->SetSize(Size);
        ShadowSlot->SetAutoSize(false);
    }
    
    // 2. 创建玻璃边框层 (如果启用)
    if (Config.bEnableGlassBorder)
    {
        Result.GlassBorder = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            *FString::Printf(TEXT("%s_GlassBorder"), *NamePrefix)
        );
        
        FLinearColor GlassBorderColor(1.0f, 1.0f, 1.0f, Config.GlassBorderOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(Result.GlassBorder, GlassBorderColor, Config.CornerRadius);
        // 增加 padding 以确保 BackgroundBlur 的矩形边缘被圆角边框覆盖
        float EffectivePadding = FMath::Max(Config.GlassBorderThickness, Config.CornerRadius * 0.3f);
        Result.GlassBorder->SetPadding(FMargin(EffectivePadding));
        // 启用裁剪，确保内容不会超出圆角边框
        Result.GlassBorder->SetClipping(EWidgetClipping::ClipToBoundsAlways);
        
        UCanvasPanelSlot* GlassSlot = ParentCanvas->AddChildToCanvas(Result.GlassBorder);
        GlassSlot->SetAnchors(AnchorPoint);
        GlassSlot->SetAlignment(Alignment);
        GlassSlot->SetPosition(Position);
        GlassSlot->SetSize(Size);
        GlassSlot->SetAutoSize(false);
    }
    
    // 3. 创建模糊层
    Result.BlurBackground = WidgetTree->ConstructWidget<UBackgroundBlur>(
        UBackgroundBlur::StaticClass(),
        *FString::Printf(TEXT("%s_Blur"), *NamePrefix)
    );
    Result.BlurBackground->SetBlurStrength(Config.BlurStrength);
    Result.BlurBackground->SetApplyAlphaToBlur(true);
    // 设置低质量回退画刷为透明，避免黑色背景
    FSlateBrush TransparentFallbackBrush2;
    TransparentFallbackBrush2.TintColor = FSlateColor(FLinearColor::Transparent);
    Result.BlurBackground->SetLowQualityFallbackBrush(TransparentFallbackBrush2);
    // 增加模糊层的 padding，使其边缘更靠内，避免超出圆角边框
    float BlurPadding = Config.ContentPadding + Config.CornerRadius * 0.15f;
    Result.BlurBackground->SetPadding(FMargin(BlurPadding));
    
    // 将模糊层添加到玻璃边框内，或直接添加到 Canvas
    if (Result.GlassBorder)
    {
        Result.GlassBorder->AddChild(Result.BlurBackground);
    }
    else
    {
        UCanvasPanelSlot* BlurSlot = ParentCanvas->AddChildToCanvas(Result.BlurBackground);
        BlurSlot->SetAnchors(AnchorPoint);
        BlurSlot->SetAlignment(Alignment);
        BlurSlot->SetPosition(Position);
        BlurSlot->SetSize(Size);
        BlurSlot->SetAutoSize(false);
    }
    
    // 4. 创建光泽层 (如果启用)
    if (Config.bEnableGloss)
    {
        Result.GlossOverlay = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            *FString::Printf(TEXT("%s_Gloss"), *NamePrefix)
        );
        
        FLinearColor GlossColor(1.0f, 1.0f, 1.0f, Config.GlossOpacity);
        MARoundedBorderUtils::ApplyRoundedCorners(Result.GlossOverlay, GlossColor, Config.CornerRadius - 2.0f);
        Result.GlossOverlay->SetPadding(FMargin(0.0f));
        
        Result.BlurBackground->AddChild(Result.GlossOverlay);
        Result.ContentContainer = Result.GlossOverlay;
    }
    else
    {
        Result.ContentContainer = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            *FString::Printf(TEXT("%s_Content"), *NamePrefix)
        );
        Result.ContentContainer->SetBrushColor(FLinearColor::Transparent);
        Result.ContentContainer->SetPadding(FMargin(0.0f));
        
        Result.BlurBackground->AddChild(Result.ContentContainer);
    }
    
    return Result;
}

FMAFrostedGlassResult MAFrostedGlassUtils::CreateFixedSizePanelFromTheme(
    UWidgetTree* WidgetTree,
    UCanvasPanel* ParentCanvas,
    const UMAUITheme* Theme,
    const FVector2D& Position,
    const FVector2D& Size,
    const FVector2D& Alignment,
    const FAnchors& AnchorPoint,
    const FString& NamePrefix)
{
    FMAFrostedGlassConfig Config = GetConfigFromTheme(Theme);
    return CreateFixedSizePanel(WidgetTree, ParentCanvas, Position, Size, Alignment, AnchorPoint, Config, NamePrefix);
}
