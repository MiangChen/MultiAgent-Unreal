// MAUITheme.h
// UI 主题数据资产 - 定义颜色、字体、间距等视觉样式变量
// 用于确保 UI 视觉一致性，便于后续调整

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "Layout/Margin.h"
#include "MAUITheme.generated.h"

/**
 * UI 主题数据资产
 * 
 * 定义所有 UI 组件使用的视觉样式变量：
 * - 颜色系统：主色、次色、危险色、成功色、警告色、背景色、文字色
 * - 字体设置：标题字体、正文字体、按钮字体、行高
 * - 形状设置：圆角半径
 * - 间距设置：内容内边距、元素间距
 * - 动画设置：淡入淡出时长、滑入时长、按钮悬停时长
 * 
 * 使用方式：
 * - 在 Content 目录下创建 Data Asset 实例
 * - 在 Widget 中引用并应用主题样式
 * 
 * Requirements: 1.1, 1.2, 1.3, 1.5
 */
UCLASS(BlueprintType)
class MULTIAGENT_API UMAUITheme : public UDataAsset
{
    GENERATED_BODY()

public:
    UMAUITheme();

    //=========================================================================
    // 颜色系统 (Requirements: 1.1)
    //=========================================================================

    /** 主色 - 用于主要按钮、强调元素 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
    FLinearColor PrimaryColor;

    /** 次色 - 用于次要按钮、边框 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
    FLinearColor SecondaryColor;

    /** 危险色 - 用于危险操作、错误提示 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
    FLinearColor DangerColor;

    /** 成功色 - 用于成功提示、确认操作 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
    FLinearColor SuccessColor;

    /** 警告色 - 用于警告提示 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
    FLinearColor WarningColor;

    /** 背景色 - 用于面板、模态窗口背景 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
    FLinearColor BackgroundColor;

    /** 文字色 - 用于主要文字 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors")
    FLinearColor TextColor;

    //=========================================================================
    // 模式颜色 (Mode Colors) - 用于不同操作模式的视觉区分
    //=========================================================================

    /** 选择模式颜色 - 绿色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Mode")
    FLinearColor ModeSelectColor;

    /** 部署模式颜色 - 蓝色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Mode")
    FLinearColor ModeDeployColor;

    /** 修改模式颜色 - 橙色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Mode")
    FLinearColor ModeModifyColor;

    /** 编辑模式颜色 - 蓝色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Mode")
    FLinearColor ModeEditColor;

    //=========================================================================
    // 状态颜色 (Status Colors) - 用于任务/技能状态显示
    //=========================================================================

    /** Pending 状态颜色 - 灰色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor StatusPendingColor;

    /** InProgress 状态颜色 - 黄色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor StatusInProgressColor;

    /** Completed 状态颜色 - 绿色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor StatusCompletedColor;

    /** Failed 状态颜色 - 红色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Status")
    FLinearColor StatusFailedColor;

    //=========================================================================
    // 文字颜色 (Text Colors) - 用于不同类型文字
    //=========================================================================

    /** 次要文字颜色 - 灰色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Text")
    FLinearColor SecondaryTextColor;

    /** 标签文字颜色 - 浅灰 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Text")
    FLinearColor LabelTextColor;

    /** 提示文字颜色 - 灰色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Text")
    FLinearColor HintTextColor;

    /** 输入框文字颜色 - 黑色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Text")
    FLinearColor InputTextColor;

    //=========================================================================
    // 画布颜色 (Canvas Colors) - 用于 DAG/Gantt 画布
    //=========================================================================

    /** 画布背景颜色 - 深色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Canvas")
    FLinearColor CanvasBackgroundColor;

    /** 网格线颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Canvas")
    FLinearColor GridLineColor;

    /** 边线颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Canvas")
    FLinearColor EdgeColor;

    /** 高亮边线颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Canvas")
    FLinearColor HighlightedEdgeColor;

    //=========================================================================
    // 交互颜色 (Interaction Colors) - 用于选中/高亮/拖放
    //=========================================================================

    /** 选中颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Interaction")
    FLinearColor SelectionColor;

    /** 高亮颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Interaction")
    FLinearColor HighlightColor;

    /** 有效放置颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Interaction")
    FLinearColor ValidDropColor;

    /** 无效放置颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Interaction")
    FLinearColor InvalidDropColor;

    //=========================================================================
    // 输入框颜色 (Input Colors)
    //=========================================================================

    /** 输入框背景颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Input")
    FLinearColor InputBackgroundColor;

    //=========================================================================
    // 遮罩颜色 (Overlay Colors)
    //=========================================================================

    /** 遮罩背景颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Overlay")
    FLinearColor OverlayColor;

    /** 阴影颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Overlay")
    FLinearColor ShadowColor;

    //=========================================================================
    // Agent 类型颜色 (Agent Colors) - 用于小地图和 Agent 列表显示
    //=========================================================================

    /** Humanoid Agent 颜色 - 黄色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Agent")
    FLinearColor AgentHumanoidColor;

    /** UAV Agent 颜色 - 蓝色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Agent")
    FLinearColor AgentUAVColor;

    /** UGV Agent 颜色 - 橙色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Agent")
    FLinearColor AgentUGVColor;

    /** Quadruped Agent 颜色 - 绿色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Agent")
    FLinearColor AgentQuadrupedColor;

    /** Agent 选中高亮颜色 - 白色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|Agent")
    FLinearColor AgentSelectedColor;

    //=========================================================================
    // 小地图颜色 (MiniMap Colors) - 用于小地图显示
    //=========================================================================

    /** 小地图背景颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|MiniMap")
    FLinearColor MiniMapBackgroundColor;

    /** 小地图相机指示器颜色 - 红色 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Colors|MiniMap")
    FLinearColor MiniMapCameraColor;

    //=========================================================================
    // 字体设置 (Requirements: 1.2)
    //=========================================================================

    /** 标题字体 - 用于模态窗口标题、重要标题 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Typography")
    FSlateFontInfo TitleFont;

    /** 正文字体 - 用于普通文本内容 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Typography")
    FSlateFontInfo BodyFont;

    /** 按钮字体 - 用于按钮文字 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Typography")
    FSlateFontInfo ButtonFont;

    /** 行高 - 文本行高倍数 (推荐 1.5-1.6) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Typography", meta = (ClampMin = "1.0", ClampMax = "3.0"))
    float LineHeight;

    //=========================================================================
    // 形状设置 (Requirements: 1.3)
    //=========================================================================

    /** 圆角半径 - 用于面板、模态窗口 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shapes", meta = (ClampMin = "0.0"))
    float CornerRadius;

    /** 按钮圆角半径 - 用于按钮 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shapes", meta = (ClampMin = "0.0"))
    float ButtonCornerRadius;

    //=========================================================================
    // 间距设置
    //=========================================================================

    /** 内容内边距 - 用于面板、模态窗口内边距 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing")
    FMargin ContentPadding;

    /** 元素间距 - 用于元素之间的间距 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spacing", meta = (ClampMin = "0.0"))
    float ElementSpacing;

    //=========================================================================
    // 动画设置
    //=========================================================================

    /** 淡入时长 (秒) - 用于模态窗口显示动画 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float FadeInDuration;

    /** 淡出时长 (秒) - 用于模态窗口隐藏动画 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float FadeOutDuration;

    /** 滑入时长 (秒) - 用于通知滑入动画 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float SlideInDuration;

    /** 按钮悬停动画时长 (秒) - 用于按钮微交互 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ButtonHoverDuration;

    //=========================================================================
    // 验证方法
    //=========================================================================

    /**
     * 验证主题完整性
     * 检查所有必需属性是否有效
     * @return true 如果主题有效
     */
    UFUNCTION(BlueprintPure, Category = "Validation")
    bool IsValid() const;

    /**
     * 验证颜色是否有效
     * @return true 如果所有颜色的 Alpha 值大于 0
     */
    UFUNCTION(BlueprintPure, Category = "Validation")
    bool HasValidColors() const;

    /**
     * 验证行高是否在有效范围内
     * @return true 如果行高在 1.5 到 1.6 之间
     */
    UFUNCTION(BlueprintPure, Category = "Validation")
    bool HasValidLineHeight() const;

    /**
     * 验证圆角半径是否有效
     * @return true 如果圆角半径为正数
     */
    UFUNCTION(BlueprintPure, Category = "Validation")
    bool HasValidCornerRadius() const;
};
