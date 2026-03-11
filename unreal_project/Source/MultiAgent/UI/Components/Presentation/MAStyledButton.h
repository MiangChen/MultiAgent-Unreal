// MAStyledButton.h
// 样式化按钮组件 - 支持主题化和微交互动画
// 用于模态窗口和其他 UI 组件中的按钮
// 纯 C++ 实现，不需要蓝图

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Application/MAStyledButtonCoordinator.h"
#include "../../Core/MARoundedBorderUtils.h"
#include "MAStyledButton.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class USizeBox;
class UMAUITheme;

//=============================================================================
// 按钮样式枚举
//=============================================================================

/**
 * 按钮样式枚举
 * 
 * 定义按钮的视觉样式：
 * - Primary: 主要按钮 (蓝色) - 用于主要操作
 * - Secondary: 次要按钮 (灰色) - 用于次要操作
 * - Danger: 危险按钮 (红色) - 用于危险/删除操作
 * - Success: 成功按钮 (绿色) - 用于确认/成功操作
 */
UENUM(BlueprintType)
enum class EMAButtonStyle : uint8
{
    /** 主要按钮 - 蓝色，用于主要操作 */
    Primary     UMETA(DisplayName = "Primary"),
    
    /** 次要按钮 - 灰色，用于次要操作 */
    Secondary   UMETA(DisplayName = "Secondary"),
    
    /** 危险按钮 - 红色，用于危险/删除操作 */
    Danger      UMETA(DisplayName = "Danger"),
    
    /** 成功按钮 - 绿色，用于确认/成功操作 */
    Success     UMETA(DisplayName = "Success"),
    
    /** 警告按钮 - 黄色，用于保存/警告操作 */
    Warning     UMETA(DisplayName = "Warning")
};

//=============================================================================
// 按钮动画状态数据结构
//=============================================================================

/**
 * 按钮动画状态
 * 
 * 存储按钮的动画状态信息
 */
USTRUCT(BlueprintType)
struct FMAButtonAnimationState
{
    GENERATED_BODY()

    /** 位置偏移 */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    FVector2D PositionOffset = FVector2D::ZeroVector;

    /** 缩放 */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    FVector2D Scale = FVector2D(1.0f, 1.0f);

    /** 颜色色调 */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    FLinearColor TintColor = FLinearColor::White;

    /** 阴影透明度 */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float ShadowOpacity = 0.3f;
};

//=============================================================================
// MAStyledButton 类
//=============================================================================

/**
 * 样式化按钮 Widget
 * 
 * 支持主题化和微交互动画的按钮组件：
 * - 支持四种样式：Primary, Secondary, Danger, Success
 * - 悬停时：颜色变化 + 向上平移 2 像素 + 阴影加深
 * - 点击时：缩放到 0.98
 * - 动画时长控制在 100ms 内
 * 
 */
UCLASS()
class MULTIAGENT_API UMAStyledButton : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAStyledButton(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 委托
    //=========================================================================

    /** 按钮点击委托 */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMAButtonClicked);

    /** 按钮点击事件 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMAButtonClicked OnClicked;

    //=========================================================================
    // 属性设置
    //=========================================================================

    /**
     * 设置按钮样式
     * @param InStyle 按钮样式
     */
    UFUNCTION(BlueprintCallable, Category = "Style")
    void SetButtonStyle(EMAButtonStyle InStyle);

    /**
     * 获取按钮样式
     * @return 当前按钮样式
     */
    UFUNCTION(BlueprintPure, Category = "Style")
    EMAButtonStyle GetButtonStyle() const { return ButtonStyle; }

    /**
     * 设置按钮文本
     * @param InText 按钮文本
     */
    UFUNCTION(BlueprintCallable, Category = "Content")
    void SetButtonText(const FText& InText);

    /**
     * 获取按钮文本
     * @return 当前按钮文本
     */
    UFUNCTION(BlueprintPure, Category = "Content")
    FText GetButtonText() const { return ButtonText; }

    /**
     * 设置按钮启用状态
     * @param bEnabled 是否启用
     */
    UFUNCTION(BlueprintCallable, Category = "State")
    void SetButtonEnabled(bool bEnabled);

    /**
     * 检查按钮是否启用
     * @return true 如果按钮启用
     */
    UFUNCTION(BlueprintPure, Category = "State")
    bool IsButtonEnabled() const { return bIsEnabled; }

    //=========================================================================
    // 主题应用
    //=========================================================================

    /**
     * 应用主题
     * @param InTheme 主题数据资产
     */
    UFUNCTION(BlueprintCallable, Category = "Theme")
    void ApplyTheme(UMAUITheme* InTheme);

    /**
     * 获取当前主题
     * @return 当前主题
     */
    UFUNCTION(BlueprintPure, Category = "Theme")
    UMAUITheme* GetTheme() const { return Theme; }

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // 配置属性
    //=========================================================================

    /** 按钮样式 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    EMAButtonStyle ButtonStyle = EMAButtonStyle::Primary;

    /** 按钮文本 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
    FText ButtonText;

    /** 主题引用 */
    UPROPERTY(EditDefaultsOnly, Category = "Theme")
    UMAUITheme* Theme;

private:
    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 根 SizeBox - 用于控制按钮大小 */
    UPROPERTY()
    USizeBox* RootSizeBox;

    /** 按钮边框 - 用于圆角和背景 */
    UPROPERTY()
    UBorder* ButtonBorder;

    /** 内部按钮 */
    UPROPERTY()
    UButton* InternalButton;

    /** 按钮文本 */
    UPROPERTY()
    UTextBlock* ButtonLabel;

    //=========================================================================
    // 状态
    //=========================================================================

    /** 是否启用 */
    bool bIsEnabled = true;

    /** 是否悬停 */
    bool bIsHovered = false;

    /** 是否按下 */
    bool bIsPressed = false;

    //=========================================================================
    // 动画状态
    //=========================================================================

    /** 当前动画状态 */
    FMAButtonAnimationState CurrentAnimState;

    /** 目标动画状态 */
    FMAButtonAnimationState TargetAnimState;

    /** 原始位置偏移 */
    FVector2D OriginalPositionOffset;

    /** 原始缩放 */
    FVector2D OriginalScale;

    /** 基础颜色 (根据样式) */
    FLinearColor BaseColor;

    /** 悬停颜色 (根据样式) */
    FLinearColor HoverColor;

    /** 动画插值速度 (用于 100ms 动画) */
    static constexpr float AnimationInterpSpeed = 10.0f;

    /** 悬停时向上平移的像素数 */
    static constexpr float HoverTranslateY = -2.0f;

    /** 按下时的缩放比例 */
    static constexpr float PressedScale = 0.98f;

    /** 悬停时阴影透明度 */
    static constexpr float HoverShadowOpacity = 0.5f;

    /** 正常阴影透明度 */
    static constexpr float NormalShadowOpacity = 0.3f;

    /** 当前按钮圆角半径 */
    float CurrentCornerRadius = MARoundedBorderUtils::DefaultButtonCornerRadius;

    /** 按钮协调器 */
    FMAStyledButtonCoordinator Coordinator;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 更新按钮颜色 (根据样式和主题) */
    void UpdateButtonColors();

    /** 更新动画状态 */
    void UpdateAnimationState(float DeltaTime);

    /** 应用动画状态到 UI */
    void ApplyAnimationState();

    /** 应用圆角效果到按钮边框 */
    void ApplyRoundedCornersToButton();

    //=========================================================================
    // 事件回调
    //=========================================================================

    /** 按钮点击回调 */
    UFUNCTION()
    void OnButtonClicked();

    /** 按钮悬停回调 */
    UFUNCTION()
    void OnButtonHovered();

    /** 按钮取消悬停回调 */
    UFUNCTION()
    void OnButtonUnhovered();

    /** 按钮按下回调 */
    UFUNCTION()
    void OnButtonPressed();

    /** 按钮释放回调 */
    UFUNCTION()
    void OnButtonReleased();
};
