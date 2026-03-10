// MASpeechBubbleWidget.h
// 通用气泡文本框组件 - 显示在 Actor 头顶的对话气泡
// 支持圆角、阴影、三角引脚，可用于突发事件消息、状态提示等任何需要展示的场景
// 纯 C++ 实现，不需要蓝图

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Application/MASpeechBubbleCoordinator.h"
#include "MASpeechBubbleWidget.generated.h"

class UBorder;
class UTextBlock;
class UCanvasPanel;
class USizeBox;
class UImage;
class UMAUITheme;

/**
 * 通用气泡文本框 Widget
 *
 * 显示在 Actor 头顶的对话气泡，类似漫画中的对话框：
 * - 宽扁的圆角矩形主体
 * - 底部三角引脚指向 robot
 * - 阴影效果
 * - 支持自动隐藏（默认 10s）
 * - 支持淡入淡出动画
 * - 通过 WidgetComponent (World Space) 附加到 Actor，跟随移动并随距离缩放
 */
UCLASS()
class MULTIAGENT_API UMASpeechBubbleWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMASpeechBubbleWidget(const FObjectInitializer& ObjectInitializer);

    /** 显示消息 */
    UFUNCTION(BlueprintCallable, Category = "SpeechBubble")
    void ShowMessage(const FString& Message, float Duration = 10.0f);

    /** 隐藏消息（带淡出动画） */
    UFUNCTION(BlueprintCallable, Category = "SpeechBubble")
    void HideMessage();

    /** 检查气泡是否可见 */
    UFUNCTION(BlueprintPure, Category = "SpeechBubble")
    bool IsBubbleVisible() const { return BubbleState.bVisible; }

    /** 获取推荐的 WidgetComponent Pivot 值（引脚尖端对齐附着点） */
    static FVector2D GetRecommendedPivot(float DrawSizeX)
    {
        float TailCenterX = TailOffsetFromLeft + TailWidth * 0.5f;
        return FVector2D(TailCenterX / DrawSizeX, 1.0f);
    }

    /** 获取推荐的 WidgetComponent DrawSize（确保能容纳气泡内容，高度尽量紧凑减少空白） */
    static FVector2D GetRecommendedDrawSize()
    {
        // 高度 = 气泡内边距*2 + 预估两行文字高度 + 引脚高度 + 少量余量
        float EstimatedHeight = BubblePaddingV * 2.0f + FontSize * 2.5f + TailHeight + 60.0f;
        return FVector2D(BubbleMaxWidth + 100.0f, EstimatedHeight);
    }

    UFUNCTION(BlueprintCallable, Category = "Theme")
    void ApplyTheme(UMAUITheme* InTheme);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void BuildUI();

    UPROPERTY() UCanvasPanel* RootCanvas = nullptr;
    UPROPERTY() UBorder* ShadowBorder = nullptr;
    UPROPERTY() UBorder* BubbleBorder = nullptr;
    UPROPERTY() USizeBox* ContentSizeBox = nullptr;
    UPROPERTY() UTextBlock* MessageText = nullptr;
    UPROPERTY() UImage* TailImage = nullptr;
    UPROPERTY() UImage* TailShadowImage = nullptr;
    UPROPERTY() UMAUITheme* Theme = nullptr;

    FMASpeechBubbleState BubbleState;
    FMASpeechBubbleCoordinator Coordinator;

    // 样式常量 - 宽扁气泡（World 空间下使用较大值，DrawSize 为 800x300）
    static constexpr float BubbleCornerRadius = 16.0f;   // 圆角半径
    static constexpr float BubblePaddingH = 28.0f;       // 气泡水平内边距
    static constexpr float BubblePaddingV = 16.0f;       // 气泡垂直内边距
    static constexpr float BubbleMaxWidth = 750.0f;      // 气泡最大宽度（超出自动换行）
    static constexpr float FontSize = 32.0f;             // 文字大小（World 空间需要较大值）
    static constexpr float FadeInDuration = 0.2f;        // 淡入动画时长（秒）
    static constexpr float FadeOutDuration = 0.25f;      // 淡出动画时长（秒）
    static constexpr float ShadowOffsetX = 5.0f;         // 阴影水平偏移
    static constexpr float ShadowOffsetY = 7.0f;         // 阴影垂直偏移
    static constexpr float TailWidth = 24.0f;            // 引脚三角形宽度
    static constexpr float TailHeight = 20.0f;           // 引脚三角形高度
    static constexpr float TailOffsetFromLeft = 60.0f;   // 引脚距气泡左侧的水平偏移
};
