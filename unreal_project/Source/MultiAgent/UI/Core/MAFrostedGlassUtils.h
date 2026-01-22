// MAFrostedGlassUtils.h
// 毛玻璃效果工具类 - 提供统一的毛玻璃容器创建方法
// 包含阴影、玻璃边框、高斯模糊、光泽叠加等效果

#pragma once

#include "CoreMinimal.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/WidgetTree.h"

class UBorder;
class UBackgroundBlur;
class UMAUITheme;

/**
 * 毛玻璃效果配置结构体
 */
struct FMAFrostedGlassConfig
{
    /** 模糊强度 (默认 25.0) */
    float BlurStrength = 25.0f;
    
    /** 窗口圆角半径 (默认 16.0) */
    float CornerRadius = 16.0f;
    
    /** 阴影偏移量 (默认 4.0) */
    float ShadowOffset = 4.0f;
    
    /** 阴影透明度 (默认 0.35) */
    float ShadowOpacity = 0.35f;
    
    /** 玻璃边框透明度 (默认 0.15) */
    float GlassBorderOpacity = 0.15f;
    
    /** 玻璃边框厚度 (默认 1.5) */
    float GlassBorderThickness = 1.5f;
    
    /** 光泽叠加透明度 (默认 0.03) */
    float GlossOpacity = 0.03f;
    
    /** 内容内边距 (默认 10.0) */
    float ContentPadding = 10.0f;
    
    /** 是否启用阴影 */
    bool bEnableShadow = true;
    
    /** 是否启用玻璃边框 */
    bool bEnableGlassBorder = true;
    
    /** 是否启用光泽效果 */
    bool bEnableGloss = true;
    
    /** 默认配置 */
    static FMAFrostedGlassConfig Default()
    {
        return FMAFrostedGlassConfig();
    }
};

/**
 * 毛玻璃效果创建结果
 */
struct FMAFrostedGlassResult
{
    /** 阴影层 (最外层) */
    UBorder* ShadowBorder = nullptr;
    
    /** 玻璃边框层 */
    UBorder* GlassBorder = nullptr;
    
    /** 模糊层 */
    UBackgroundBlur* BlurBackground = nullptr;
    
    /** 光泽层 */
    UBorder* GlossOverlay = nullptr;
    
    /** 内容容器 (最内层，用于添加内容) */
    UBorder* ContentContainer = nullptr;
    
    /** 是否创建成功 */
    bool IsValid() const { return ContentContainer != nullptr; }
};

/**
 * 毛玻璃效果工具类
 * 
 * 提供静态方法创建毛玻璃效果容器，包含：
 * - 阴影层：带圆角的半透明黑色边框，偏移显示
 * - 玻璃边框：带圆角的半透明白色边框，模拟玻璃厚度
 * - 模糊层：UBackgroundBlur 实现高斯模糊
 * - 光泽层：极淡的白色叠加，增加玻璃质感
 * - 内容容器：最内层，用于添加实际内容
 * 
 * 使用示例：
 * @code
 * // 方式1: 使用默认配置
 * auto Result = MAFrostedGlassUtils::CreateFrostedGlassContainer(
 *     WidgetTree, RootCanvas,
 *     FAnchors(0.1f, 0.1f, 0.9f, 0.9f),
 *     FMargin(0.0f)
 * );
 * Result.ContentContainer->AddChild(MyContent);
 * 
 * // 方式2: 使用主题配置
 * auto Result = MAFrostedGlassUtils::CreateFromTheme(
 *     WidgetTree, RootCanvas, Theme,
 *     FAnchors(0.1f, 0.1f, 0.9f, 0.9f),
 *     FMargin(0.0f)
 * );
 * @endcode
 */
class MULTIAGENT_API MAFrostedGlassUtils
{
public:
    /**
     * 创建毛玻璃效果容器
     * 
     * @param WidgetTree Widget 树
     * @param ParentCanvas 父级 CanvasPanel
     * @param Anchors 锚点设置
     * @param Offsets 偏移设置
     * @param Config 毛玻璃配置
     * @param NamePrefix 组件名称前缀 (用于区分多个毛玻璃容器)
     * @return 创建结果，包含各层级的引用
     */
    static FMAFrostedGlassResult CreateFrostedGlassContainer(
        UWidgetTree* WidgetTree,
        UCanvasPanel* ParentCanvas,
        const FAnchors& Anchors,
        const FMargin& Offsets,
        const FMAFrostedGlassConfig& Config = FMAFrostedGlassConfig::Default(),
        const FString& NamePrefix = TEXT("FrostedGlass")
    );
    
    /**
     * 从主题创建毛玻璃效果容器
     * 
     * @param WidgetTree Widget 树
     * @param ParentCanvas 父级 CanvasPanel
     * @param Theme UI 主题 (如果为 null 则使用默认配置)
     * @param Anchors 锚点设置
     * @param Offsets 偏移设置
     * @param NamePrefix 组件名称前缀
     * @return 创建结果
     */
    static FMAFrostedGlassResult CreateFromTheme(
        UWidgetTree* WidgetTree,
        UCanvasPanel* ParentCanvas,
        const UMAUITheme* Theme,
        const FAnchors& Anchors,
        const FMargin& Offsets,
        const FString& NamePrefix = TEXT("FrostedGlass")
    );
    
    /**
     * 从主题获取毛玻璃配置
     * 
     * @param Theme UI 主题 (如果为 null 则返回默认配置)
     * @return 毛玻璃配置
     */
    static FMAFrostedGlassConfig GetConfigFromTheme(const UMAUITheme* Theme);
    
    /**
     * 为固定尺寸面板创建毛玻璃效果容器
     * 适用于侧边栏等使用固定位置和尺寸的面板
     * 
     * @param WidgetTree Widget 树
     * @param ParentCanvas 父级 CanvasPanel
     * @param Position 面板位置
     * @param Size 面板尺寸
     * @param Alignment 对齐方式
     * @param AnchorPoint 锚点
     * @param Config 毛玻璃配置
     * @param NamePrefix 组件名称前缀
     * @return 创建结果
     */
    static FMAFrostedGlassResult CreateFixedSizePanel(
        UWidgetTree* WidgetTree,
        UCanvasPanel* ParentCanvas,
        const FVector2D& Position,
        const FVector2D& Size,
        const FVector2D& Alignment = FVector2D(0.0f, 0.0f),
        const FAnchors& AnchorPoint = FAnchors(0.0f, 0.0f, 0.0f, 0.0f),
        const FMAFrostedGlassConfig& Config = FMAFrostedGlassConfig::Default(),
        const FString& NamePrefix = TEXT("Panel")
    );
    
    /**
     * 从主题为固定尺寸面板创建毛玻璃效果容器
     * 
     * @param WidgetTree Widget 树
     * @param ParentCanvas 父级 CanvasPanel
     * @param Theme UI 主题
     * @param Position 面板位置
     * @param Size 面板尺寸
     * @param Alignment 对齐方式
     * @param AnchorPoint 锚点
     * @param NamePrefix 组件名称前缀
     * @return 创建结果
     */
    static FMAFrostedGlassResult CreateFixedSizePanelFromTheme(
        UWidgetTree* WidgetTree,
        UCanvasPanel* ParentCanvas,
        const UMAUITheme* Theme,
        const FVector2D& Position,
        const FVector2D& Size,
        const FVector2D& Alignment = FVector2D(0.0f, 0.0f),
        const FAnchors& AnchorPoint = FAnchors(0.0f, 0.0f, 0.0f, 0.0f),
        const FString& NamePrefix = TEXT("Panel")
    );

private:
    /** 禁止实例化 */
    MAFrostedGlassUtils() = delete;
};
