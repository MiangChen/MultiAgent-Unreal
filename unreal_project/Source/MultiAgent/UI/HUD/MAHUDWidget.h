// MAHUDWidget.h
// HUD Widget - 承载原 Canvas 绘制的 HUD 元素
// 包括 Edit 模式指示器、通知消息
// 纯 C++ 实现，不需要蓝图

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MAHUDWidget.generated.h"

class UTextBlock;
class UCanvasPanel;
class UMAUITheme;

/**
 * HUD Widget
 * 
 * 承载原 Canvas 绘制的 HUD 元素，解决 HUD 透过 Widget 显示的问题
 * 
 * 组件:
 * - EditModeText: 右上角蓝色 "Mode: Edit (M)" 文字
 * - POIListText: 左下角绿色 POI 列表
 * - GoalListText: 左下角红色 Goal 列表
 * - ZoneListText: 左下角青色 Zone 列表
 * - NotificationText: 底部中央通知消息
 */
UCLASS()
class MULTIAGENT_API UMAHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAHUDWidget(const FObjectInitializer& ObjectInitializer);

    // Edit 模式指示器
    //=========================================================================

    /**
     * 设置 Edit 模式指示器可见性
     * @param bVisible 是否可见
     */
    UFUNCTION(BlueprintCallable, Category = "HUD|EditMode")
    void SetEditModeVisible(bool bVisible);

    /**
     * 更新 POI 列表
     * @param POIInfos POI 信息字符串数组
     */
    UFUNCTION(BlueprintCallable, Category = "HUD|EditMode")
    void UpdatePOIList(const TArray<FString>& POIInfos);

    /**
     * 更新 Goal 列表
     * @param GoalInfos Goal 信息字符串数组
     */
    UFUNCTION(BlueprintCallable, Category = "HUD|EditMode")
    void UpdateGoalList(const TArray<FString>& GoalInfos);

    /**
     * 更新 Zone 列表
     * @param ZoneInfos Zone 信息字符串数组
     */
    UFUNCTION(BlueprintCallable, Category = "HUD|EditMode")
    void UpdateZoneList(const TArray<FString>& ZoneInfos);

    //=========================================================================
    // 通知消息
    //=========================================================================

    /**
     * 显示通知消息
     * @param Message 消息内容
     * @param bIsError 是否为错误消息 (红色)
     * @param bIsWarning 是否为警告消息 (黄色)，优先级低于 bIsError
     */
    UFUNCTION(BlueprintCallable, Category = "HUD|Notification")
    void ShowNotification(const FString& Message, bool bIsError = false, bool bIsWarning = false);

    //=========================================================================
    // 主题系统
    //=========================================================================

    /**
     * 应用主题样式
     * @param InTheme 主题数据资产
     */
    void ApplyTheme(UMAUITheme* InTheme);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeDestruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    //=========================================================================
    // 主题
    //=========================================================================

    /** 缓存的主题引用 */
    UPROPERTY()
    UMAUITheme* Theme;

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 根 Canvas */
    UPROPERTY()
    UCanvasPanel* RootCanvas;


    /** Edit 模式指示器文本 (右上角，蓝色) */
    UPROPERTY()
    UTextBlock* EditModeText;

    /** POI 列表文本 (左下角，绿色) */
    UPROPERTY()
    UTextBlock* POIListText;

    /** Goal 列表文本 (左下角，红色) */
    UPROPERTY()
    UTextBlock* GoalListText;

    /** Zone 列表文本 (左下角，青色) */
    UPROPERTY()
    UTextBlock* ZoneListText;

    /** 通知消息文本 (底部中央) */
    UPROPERTY()
    UTextBlock* NotificationText;

    //=========================================================================
    // 通知状态
    //=========================================================================

    /** 通知淡出 Timer */
    FTimerHandle NotificationTimerHandle;

    /** 通知显示持续时间 (秒) */
    static constexpr float NotificationDisplayDuration = 2.5f;

    /** 通知淡出持续时间 (秒) */
    static constexpr float NotificationFadeDuration = 0.5f;

    /** 淡出动画 Timer */
    FTimerHandle FadeTimerHandle;

    /** 当前淡出进度 */
    float CurrentFadeAlpha;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 开始通知淡出 */
    void StartNotificationFadeOut();

    /** 淡出动画 Tick */
    void OnFadeTick();

    /** 通知淡出完成 */
    void OnNotificationFadeComplete();
};
