// MANotificationWidget.h
// 通知组件 - 显示后端更新通知，支持滑入动画
// 用于在小地图下方显示任务图、技能列表、突发事件的更新提示
// 纯 C++ 实现，不需要蓝图

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Core/MAHUDTypes.h"
#include "MANotificationWidget.generated.h"

class UBorder;
class UTextBlock;
class UCanvasPanel;
class UVerticalBox;
class UMAUITheme;

//=============================================================================
// MANotificationWidget 类
//=============================================================================

/**
 * 通知组件 Widget
 * 
 * 显示后端更新通知，支持滑入/滑出动画：
 * - 任务图更新: "Press 'Z' to review Task Graph"
 * - 技能列表更新: "Press 'N' to review Skill List"
 * - 突发事件: "Press 'X' to check Unexpected Situation"
 * 
 * 动画效果：
 * - 滑入动画：从上方滑入，300ms
 * - 滑出动画：向上滑出
 * - 通知持续显示直到用户按下对应键
 * 
 * Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6
 */
UCLASS()
class MULTIAGENT_API UMANotificationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMANotificationWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 通知控制 (Requirements: 4.1, 4.2, 4.3)
    //=========================================================================

    /**
     * 显示通知
     * @param Type 通知类型
     */
    UFUNCTION(BlueprintCallable, Category = "Notification")
    void ShowNotification(EMANotificationType Type);

    /**
     * 隐藏通知
     */
    UFUNCTION(BlueprintCallable, Category = "Notification")
    void HideNotification();

    /**
     * 检查通知是否可见
     * @return true 如果通知正在显示
     */
    UFUNCTION(BlueprintPure, Category = "Notification")
    bool IsNotificationVisible() const { return bIsVisible; }

    /**
     * 获取当前通知类型
     * @return 当前通知类型
     */
    UFUNCTION(BlueprintPure, Category = "Notification")
    EMANotificationType GetCurrentNotificationType() const { return CurrentType; }

    //=========================================================================
    // 消息获取 (Requirements: 4.1, 4.2, 4.3)
    //=========================================================================

    /**
     * 获取通知消息
     * @param Type 通知类型
     * @return 对应的通知消息
     */
    UFUNCTION(BlueprintPure, Category = "Notification")
    static FString GetNotificationMessage(EMANotificationType Type);

    /**
     * 获取按键提示
     * @param Type 通知类型
     * @return 对应的按键提示
     */
    UFUNCTION(BlueprintPure, Category = "Notification")
    static FString GetKeyHint(EMANotificationType Type);

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
    // UI 组件
    //=========================================================================

    /** 根 Canvas */
    UPROPERTY()
    UCanvasPanel* RootCanvas;

    /** 通知边框 (圆角背景) */
    UPROPERTY()
    UBorder* NotificationBorder;

    /** 内容垂直布局 */
    UPROPERTY()
    UVerticalBox* ContentBox;

    /** 通知消息文本 */
    UPROPERTY()
    UTextBlock* NotificationText;

    /** 按键提示文本 */
    UPROPERTY()
    UTextBlock* KeyHintText;

    //=========================================================================
    // 配置属性
    //=========================================================================

    /** 主题引用 */
    UPROPERTY(EditDefaultsOnly, Category = "Theme")
    UMAUITheme* Theme;

    /** 通知宽度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float NotificationWidth = 280.0f;

    /** 通知内边距 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float NotificationPadding = 12.0f;

    /** 圆角半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
    float CornerRadius = 8.0f;

private:
    //=========================================================================
    // 状态
    //=========================================================================

    /** 当前通知类型 */
    EMANotificationType CurrentType = EMANotificationType::None;

    /** 是否可见 */
    bool bIsVisible = false;

    /** 是否正在播放动画 */
    bool bIsAnimating = false;

    /** 是否正在显示 (用于动画方向) */
    bool bIsShowing = false;

    //=========================================================================
    // 动画状态 (Requirements: 4.4, 4.6)
    //=========================================================================

    /** 当前动画时间 */
    float CurrentAnimTime = 0.0f;

    /** 滑入动画时长 (秒) - 300ms */
    static constexpr float SlideInDuration = 0.3f;

    /** 滑出动画时长 (秒) */
    static constexpr float SlideOutDuration = 0.2f;

    /** 动画起始 X 偏移 (从左边缘滑入) */
    static constexpr float AnimStartOffsetX = -300.0f;

    /** 动画目标 X 偏移 (最终位置) */
    static constexpr float AnimEndOffsetX = 0.0f;

    /** 当前 X 偏移 */
    float CurrentOffsetX = AnimStartOffsetX;

    /** 当前透明度 */
    float CurrentOpacity = 0.0f;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 更新通知内容 */
    void UpdateNotificationContent();

    /** 更新动画状态 */
    void UpdateAnimation(float DeltaTime);

    /** 应用动画状态到 UI */
    void ApplyAnimationState();

    /** 动画完成回调 */
    void OnAnimationFinished();

    /** 获取通知类型对应的背景颜色 */
    FLinearColor GetBackgroundColorForType(EMANotificationType Type) const;
};
