// MADirectControlIndicator.h
// Direct Control 指示器 Widget - 显示当前直接控制的 Agent 名称

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MADirectControlIndicator.generated.h"

class UTextBlock;
class UBorder;
class UCanvasPanel;

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
     * @param AgentName Agent 的名称
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetAgentName(const FString& AgentName);

    /**
     * 获取当前显示的 Agent 名称
     * @return Agent 名称
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    FString GetAgentName() const;

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

    /** 当前显示的 Agent 名称 */
    FString CurrentAgentName;
};
