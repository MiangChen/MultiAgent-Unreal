// MADecisionModal.h
// HITL 决策模态窗口 - 用于显示技能执行结果并让操作员做出 Yes/No 决策
// 继承自 MABaseModalWidget，提供描述区 + 用户输入区 + Yes/No 按钮

#pragma once

#include "CoreMinimal.h"
#include "MABaseModalWidget.h"
#include "MADecisionModal.generated.h"

class UTextBlock;
class UVerticalBox;
class UBorder;
class UMultiLineEditableTextBox;

//=============================================================================
// 委托声明
//=============================================================================

/** 决策确认委托 (Yes - end_task) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDecisionConfirmed, const FString&, SelectedOption, const FString&, UserFeedback);

/** 决策拒绝委托 (No - continue_task) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDecisionRejected, const FString&, SelectedOption, const FString&, UserFeedback);

//=============================================================================
// UMADecisionModal - HITL 决策模态窗口
//=============================================================================

/**
 * HITL 决策模态窗口
 *
 * 功能:
 * - 显示来自 Python 端的决策描述文本
 * - 提供可选的用户反馈输入框
 * - Yes/No 按钮用于结束或继续任务
 * - 直接进入编辑模式（用户需要操作按钮）
 *
 * 布局结构:
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                        Decision Required                            │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │  ┌─────────────────────────────────────────────────────────────┐   │
 * │  │  Description Text (from Python)                              │   │
 * │  └─────────────────────────────────────────────────────────────┘   │
 * │  ┌─────────────────────────────────────────────────────────────┐   │
 * │  │  User Feedback (optional input)                              │   │
 * │  │  [Multi-line text input box]                                 │   │
 * │  └─────────────────────────────────────────────────────────────┘   │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │                              [Yes] [No]                             │
 * └─────────────────────────────────────────────────────────────────────┘
 */
UCLASS()
class MULTIAGENT_API UMADecisionModal : public UMABaseModalWidget
{
    GENERATED_BODY()

public:
    UMADecisionModal(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 加载决策数据
     * @param Description 决策描述文本 (来自 Python 端)
     * @param ContextJson 上下文 JSON 字符串
     */
    UFUNCTION(BlueprintCallable, Category = "DecisionModal")
    void LoadDecisionData(const FString& Description, const FString& ContextJson);

    /**
     * 获取用户反馈文本
     * @return 用户输入的反馈文本 (可能为空)
     */
    UFUNCTION(BlueprintPure, Category = "DecisionModal")
    FString GetUserFeedback() const;

    //=========================================================================
    // 委托
    //=========================================================================

    /** 决策确认委托 (Yes → end_task) */
    UPROPERTY(BlueprintAssignable, Category = "DecisionModal|Events")
    FOnDecisionConfirmed OnDecisionConfirmed;

    /** 决策拒绝委托 (No → continue_task) */
    UPROPERTY(BlueprintAssignable, Category = "DecisionModal|Events")
    FOnDecisionRejected OnDecisionRejected;

protected:
    //=========================================================================
    // UMABaseModalWidget 重写
    //=========================================================================

    virtual FText GetModalTitleText() const override;
    virtual void BuildContentArea(UVerticalBox* InContentContainer) override;
    virtual void OnThemeApplied() override;

    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 创建描述区域 */
    UBorder* CreateDescriptionArea();

    /** 创建用户反馈输入区域 */
    UBorder* CreateUserFeedbackArea();

    /** 处理确认按钮点击 (Yes → end_task) */
    UFUNCTION()
    void HandleConfirm();

    /** 处理拒绝按钮点击 (No → continue_task) */
    UFUNCTION()
    void HandleReject();

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 描述容器 */
    UPROPERTY()
    UBorder* DescriptionContainer;

    /** 描述文本 */
    UPROPERTY()
    UTextBlock* DescriptionText;

    /** 用户反馈容器 */
    UPROPERTY()
    UBorder* FeedbackContainer;

    /** 用户反馈标签 */
    UPROPERTY()
    UTextBlock* FeedbackLabel;

    /** 用户反馈输入框 */
    UPROPERTY()
    UMultiLineEditableTextBox* FeedbackInput;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 当前决策描述 */
    FString CurrentDescription;

    /** 当前上下文 JSON */
    FString CurrentContextJson;
};
