// MADirectControlIndicator.h
// Direct Control 指示器 Widget - 显示当前直接控制的 Agent 名称

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Application/MADirectControlIndicatorCoordinator.h"
#include "MADirectControlIndicator.generated.h"

class UTextBlock;
class UBorder;
class UCanvasPanel;
class UMAUITheme;

/**
 * Direct Control 指示器 Widget
 * 
 * 在 Agent View Mode 时显示 "Direct Control: [Agent Name]"
 * 纯 C++ 实现，不需要蓝图
 * 
 */
UCLASS()
class MULTIAGENT_API UMADirectControlIndicator : public UUserWidget
{
    GENERATED_BODY()

public:
    UMADirectControlIndicator(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 设置显示的 Agent 名称
     * @param AgentLabel Agent 的名称
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetAgentLabel(const FString& AgentLabel);

    /**
     * 获取当前显示的 Agent 名称
     * @return Agent 名称
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    FString GetAgentLabel() const;

    /**
     * 应用主题样式
     * @param InTheme 主题数据资产
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Theme")
    void ApplyTheme(UMAUITheme* InTheme);

protected:
    virtual void NativePreConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // UI 控件 (动态创建)
    //=========================================================================

    UPROPERTY()
    UTextBlock* IndicatorText;

    UPROPERTY()
    UBorder* BackgroundBorder;

private:
    /** 构建 UI 布局 */
    void BuildUI();

    /** 应用显示模型 */
    void ApplyModel(const FMADirectControlIndicatorModel& Model);

    /** 当前显示的 Agent 名称 */
    FString CurrentAgentLabel;

    /** 缓存的主题引用 */
    UPROPERTY()
    UMAUITheme* Theme;

    /** 指示器文字颜色 (默认绿色) */
    FLinearColor IndicatorTextColor = FLinearColor(0.2f, 1.0f, 0.2f);

    /** 指示器协调器 */
    FMADirectControlIndicatorCoordinator Coordinator;
};
