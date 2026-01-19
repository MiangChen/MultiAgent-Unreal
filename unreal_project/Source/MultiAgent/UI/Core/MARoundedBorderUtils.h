// MARoundedBorderUtils.h
// 圆角边框工具类 - 提供统一的圆角效果创建和应用方法
// 使用 UE5 的 FSlateRoundedBoxBrush 实现圆角矩形渲染

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "MARoundedBorderUtils.generated.h"

class UBorder;
class UMAUITheme;

//=============================================================================
// 圆角元素类型枚举
//=============================================================================

/**
 * 圆角边框元素类型
 * 
 * 用于指定不同 UI 元素应使用的圆角半径类型
 * - Panel: 面板/模态窗口，使用 CornerRadius (默认 12.0f)
 * - Button: 按钮，使用 ButtonCornerRadius (默认 8.0f)
 * - Custom: 自定义，使用指定的圆角半径值
 * 
 * Requirements: 1.4
 */
UENUM(BlueprintType)
enum class EMARoundedElementType : uint8
{
    /** 面板/模态窗口 - 使用 CornerRadius */
    Panel       UMETA(DisplayName = "Panel"),
    
    /** 按钮 - 使用 ButtonCornerRadius */
    Button      UMETA(DisplayName = "Button"),
    
    /** 自定义 - 使用指定的圆角半径 */
    Custom      UMETA(DisplayName = "Custom")
};

//=============================================================================
// MARoundedBorderUtils 工具类
//=============================================================================

/**
 * 圆角边框工具类
 * 
 * 提供静态方法创建和应用圆角效果到 UBorder Widget。
 * 使用 UE5 的 FSlateRoundedBoxBrush 实现高性能圆角渲染。
 * 
 * 主要功能：
 * - CreateRoundedBoxBrush: 创建圆角矩形画刷
 * - ApplyRoundedCorners: 应用圆角效果到 UBorder
 * - ApplyRoundedCornersFromTheme: 从主题获取圆角值并应用
 * - GetCornerRadiusForType: 获取指定类型的圆角半径
 * 
 * 使用示例：
 * @code
 * // 方式1: 直接指定圆角半径
 * MARoundedBorderUtils::ApplyRoundedCorners(MyBorder, FLinearColor::Black, 12.0f);
 * 
 * // 方式2: 从主题获取圆角半径
 * MARoundedBorderUtils::ApplyRoundedCornersFromTheme(MyBorder, Theme, EMARoundedElementType::Panel);
 * @endcode
 * 
 * Requirements: 1.1, 1.2, 1.3, 1.4, 1.5
 */
class MULTIAGENT_API MARoundedBorderUtils
{
public:
    //=========================================================================
    // 默认值常量
    //=========================================================================

    /** 默认面板圆角半径 */
    static constexpr float DefaultPanelCornerRadius = 12.0f;
    
    /** 默认按钮圆角半径 */
    static constexpr float DefaultButtonCornerRadius = 8.0f;

    //=========================================================================
    // 画刷创建方法
    //=========================================================================

    /**
     * 创建圆角矩形画刷 (四角相同半径)
     * 
     * @param Color 背景颜色
     * @param CornerRadius 圆角半径 (所有四个角相同)
     * @return 配置好的 FSlateBrush
     * 
     * Requirements: 1.1
     */
    static FSlateBrush CreateRoundedBoxBrush(
        const FLinearColor& Color, 
        float CornerRadius);
    
    /**
     * 创建圆角矩形画刷 (支持不同角的半径)
     * 
     * @param Color 背景颜色
     * @param TopLeft 左上角半径
     * @param TopRight 右上角半径
     * @param BottomRight 右下角半径
     * @param BottomLeft 左下角半径
     * @return 配置好的 FSlateBrush
     * 
     * Requirements: 1.1
     */
    static FSlateBrush CreateRoundedBoxBrush(
        const FLinearColor& Color,
        float TopLeft,
        float TopRight,
        float BottomRight,
        float BottomLeft);

    //=========================================================================
    // 圆角应用方法
    //=========================================================================

    /**
     * 应用圆角效果到 UBorder
     * 
     * @param Border 目标 Border Widget (如果为 null 则不执行任何操作)
     * @param Color 背景颜色
     * @param CornerRadius 圆角半径
     * 
     * Requirements: 1.2
     */
    static void ApplyRoundedCorners(
        UBorder* Border,
        const FLinearColor& Color,
        float CornerRadius);
    
    /**
     * 从主题应用圆角效果到 UBorder
     * 
     * @param Border 目标 Border Widget (如果为 null 则不执行任何操作)
     * @param Theme UI 主题 (如果为 null 则使用默认值)
     * @param ElementType 元素类型 (决定使用哪个圆角半径)
     * @param CustomRadius 自定义圆角半径 (仅当 ElementType 为 Custom 时使用)
     * 
     * Requirements: 1.2, 1.5
     */
    static void ApplyRoundedCornersFromTheme(
        UBorder* Border,
        const UMAUITheme* Theme,
        EMARoundedElementType ElementType,
        float CustomRadius = 0.0f);

    //=========================================================================
    // 辅助方法
    //=========================================================================

    /**
     * 获取主题中指定元素类型的圆角半径
     * 
     * @param Theme UI 主题 (如果为 null 则返回默认值)
     * @param ElementType 元素类型
     * @return 圆角半径值
     * 
     * Requirements: 1.4, 1.5
     */
    static float GetCornerRadiusForType(
        const UMAUITheme* Theme,
        EMARoundedElementType ElementType);

private:
    /** 禁止实例化 */
    MARoundedBorderUtils() = delete;
};
