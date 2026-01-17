// MAUITheme.cpp
// UI 主题数据资产实现

#include "MAUITheme.h"

UMAUITheme::UMAUITheme()
{
    //=========================================================================
    // 默认颜色值
    //=========================================================================
    
    // 主色 - 蓝色
    PrimaryColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
    
    // 次色 - 深灰
    SecondaryColor = FLinearColor(0.3f, 0.3f, 0.35f, 1.0f);
    
    // 危险色 - 红色
    DangerColor = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);
    
    // 成功色 - 绿色
    SuccessColor = FLinearColor(0.3f, 0.8f, 0.4f, 1.0f);
    
    // 警告色 - 黄色
    WarningColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);
    
    // 背景色 - 深色半透明
    BackgroundColor = FLinearColor(0.1f, 0.1f, 0.12f, 0.95f);
    
    // 文字色 - 白色
    TextColor = FLinearColor(0.95f, 0.95f, 0.95f, 1.0f);

    //=========================================================================
    // 默认字体设置
    //=========================================================================
    
    // 标题字体 - 使用 Roboto Bold, Size 24
    // 注意: Inter 字体需要单独导入，这里使用引擎默认字体
    TitleFont = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 24);
    
    // 正文字体 - 使用 Roboto Regular, Size 14
    BodyFont = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14);
    
    // 按钮字体 - 使用 Roboto Medium, Size 14
    ButtonFont = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14);
    
    // 行高 - 推荐 1.5-1.6
    LineHeight = 1.5f;

    //=========================================================================
    // 默认形状设置
    //=========================================================================
    
    // 圆角半径 - 面板、模态窗口
    CornerRadius = 12.0f;
    
    // 按钮圆角半径
    ButtonCornerRadius = 8.0f;

    //=========================================================================
    // 默认间距设置
    //=========================================================================
    
    // 内容内边距
    ContentPadding = FMargin(20.0f);
    
    // 元素间距
    ElementSpacing = 12.0f;

    //=========================================================================
    // 默认动画设置
    //=========================================================================
    
    // 淡入时长 - 200ms
    FadeInDuration = 0.2f;
    
    // 淡出时长 - 200ms
    FadeOutDuration = 0.2f;
    
    // 滑入时长 - 300ms
    SlideInDuration = 0.3f;
    
    // 按钮悬停动画时长 - 100ms
    ButtonHoverDuration = 0.1f;
}

bool UMAUITheme::IsValid() const
{
    return HasValidColors() && HasValidLineHeight() && HasValidCornerRadius();
}

bool UMAUITheme::HasValidColors() const
{
    return PrimaryColor.A > 0.0f
        && SecondaryColor.A > 0.0f
        && DangerColor.A > 0.0f
        && SuccessColor.A > 0.0f
        && WarningColor.A > 0.0f
        && BackgroundColor.A > 0.0f
        && TextColor.A > 0.0f;
}

bool UMAUITheme::HasValidLineHeight() const
{
    return LineHeight >= 1.5f && LineHeight <= 1.6f;
}

bool UMAUITheme::HasValidCornerRadius() const
{
    return CornerRadius > 0.0f && ButtonCornerRadius > 0.0f;
}
