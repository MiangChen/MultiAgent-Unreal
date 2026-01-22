// MAUITheme.cpp
// UI 主题数据资产实现

#include "MAUITheme.h"
#include "Styling/CoreStyle.h"

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
    
    // 文字色 - 黑色
    TextColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);

    //=========================================================================
    // 模式颜色默认值
    //=========================================================================
    
    // 选择模式 - 绿色
    ModeSelectColor = FLinearColor::Green;
    
    // 部署模式 - 蓝色
    ModeDeployColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
    
    // 修改模式 - 橙色
    ModeModifyColor = FLinearColor(1.0f, 0.6f, 0.0f, 1.0f);
    
    // 编辑模式 - 蓝色
    ModeEditColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);

    //=========================================================================
    // 状态颜色默认值
    //=========================================================================
    
    // Pending - 灰色
    StatusPendingColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);
    
    // InProgress - 黄色
    StatusInProgressColor = FLinearColor(0.9f, 0.8f, 0.2f, 1.0f);
    
    // Completed - 绿色
    StatusCompletedColor = FLinearColor(0.2f, 0.8f, 0.3f, 1.0f);
    
    // Failed - 红色
    StatusFailedColor = FLinearColor(0.9f, 0.2f, 0.2f, 1.0f);

    //=========================================================================
    // 文字颜色默认值
    //=========================================================================
    
    // 次要文字 - 灰色
    SecondaryTextColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
    
    // 标签文字 - 浅灰
    LabelTextColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
    
    // 提示文字 - 灰色
    HintTextColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
    
    // 输入框文字 - 黑色
    InputTextColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);

    //=========================================================================
    // 画布颜色默认值
    //=========================================================================
    
    // 画布背景 - 深色
    CanvasBackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);
    
    // 网格线
    GridLineColor = FLinearColor(0.15f, 0.15f, 0.2f, 1.0f);
    
    // 边线
    EdgeColor = FLinearColor(0.5f, 0.5f, 0.6f, 1.0f);
    
    // 高亮边线
    HighlightedEdgeColor = FLinearColor(0.3f, 0.8f, 0.3f, 1.0f);

    //=========================================================================
    // 交互颜色默认值
    //=========================================================================
    
    // 选中颜色
    SelectionColor = FLinearColor(0.3f, 0.7f, 1.0f, 1.0f);
    
    // 高亮颜色
    HighlightColor = FLinearColor(0.2f, 0.2f, 0.3f, 0.95f);
    
    // 有效放置
    ValidDropColor = FLinearColor(0.2f, 0.8f, 0.3f, 0.5f);
    
    // 无效放置
    InvalidDropColor = FLinearColor(0.9f, 0.2f, 0.2f, 0.5f);

    //=========================================================================
    // 输入框颜色默认值
    //=========================================================================
    
    // 输入框背景 - 白色
    InputBackgroundColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    //=========================================================================
    // 遮罩颜色默认值
    //=========================================================================
    
    // 遮罩背景
    OverlayColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
    
    // 阴影
    ShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);

    //=========================================================================
    // Agent 类型颜色默认值
    //=========================================================================
    
    // Humanoid - 黄色
    AgentHumanoidColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);
    
    // UAV - 蓝色
    AgentUAVColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
    
    // UGV - 橙色
    AgentUGVColor = FLinearColor(0.8f, 0.4f, 0.1f, 1.0f);
    
    // Quadruped - 绿色
    AgentQuadrupedColor = FLinearColor(0.2f, 1.0f, 0.4f, 1.0f);
    
    // Agent 选中 - 白色
    AgentSelectedColor = FLinearColor::White;

    //=========================================================================
    // 小地图颜色默认值
    //=========================================================================
    
    // 小地图背景 - 深色半透明
    MiniMapBackgroundColor = FLinearColor(0.1f, 0.1f, 0.15f, 0.85f);
    
    // 小地图相机指示器 - 红色
    MiniMapCameraColor = FLinearColor(1.0f, 0.2f, 0.2f, 1.0f);

    //=========================================================================
    // 默认字体设置
    //=========================================================================
    
    // 标题字体 - 使用引擎默认 Bold 字体, Size 24
    TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 24);
    
    // 正文字体 - 使用引擎默认 Regular 字体, Size 14
    BodyFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
    
    // 按钮字体 - 使用引擎默认 Regular 字体, Size 14
    ButtonFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
    
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
    // 基础颜色验证
    bool bBaseColorsValid = PrimaryColor.A > 0.0f
        && SecondaryColor.A > 0.0f
        && DangerColor.A > 0.0f
        && SuccessColor.A > 0.0f
        && WarningColor.A > 0.0f
        && BackgroundColor.A > 0.0f
        && TextColor.A > 0.0f;
    
    // 模式颜色验证
    bool bModeColorsValid = ModeSelectColor.A > 0.0f
        && ModeDeployColor.A > 0.0f
        && ModeModifyColor.A > 0.0f
        && ModeEditColor.A > 0.0f;
    
    // 状态颜色验证
    bool bStatusColorsValid = StatusPendingColor.A > 0.0f
        && StatusInProgressColor.A > 0.0f
        && StatusCompletedColor.A > 0.0f
        && StatusFailedColor.A > 0.0f;
    
    // 文字颜色验证
    bool bTextColorsValid = SecondaryTextColor.A > 0.0f
        && LabelTextColor.A > 0.0f
        && HintTextColor.A > 0.0f
        && InputTextColor.A > 0.0f;
    
    // 画布颜色验证
    bool bCanvasColorsValid = CanvasBackgroundColor.A > 0.0f
        && GridLineColor.A > 0.0f
        && EdgeColor.A > 0.0f
        && HighlightedEdgeColor.A > 0.0f;
    
    // 交互颜色验证
    bool bInteractionColorsValid = SelectionColor.A > 0.0f
        && HighlightColor.A > 0.0f
        && ValidDropColor.A > 0.0f
        && InvalidDropColor.A > 0.0f;
    
    // 输入框和遮罩颜色验证
    bool bInputOverlayColorsValid = InputBackgroundColor.A > 0.0f
        && OverlayColor.A > 0.0f
        && ShadowColor.A > 0.0f;
    
    return bBaseColorsValid 
        && bModeColorsValid 
        && bStatusColorsValid 
        && bTextColorsValid 
        && bCanvasColorsValid 
        && bInteractionColorsValid 
        && bInputOverlayColorsValid;
}

bool UMAUITheme::HasValidLineHeight() const
{
    return LineHeight >= 1.5f && LineHeight <= 1.6f;
}

bool UMAUITheme::HasValidCornerRadius() const
{
    return CornerRadius > 0.0f && ButtonCornerRadius > 0.0f;
}
