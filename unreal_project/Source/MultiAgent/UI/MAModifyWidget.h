// MAModifyWidget.h
// Modify 模式修改面板 Widget - 纯 C++ 实现
// 用于查看和编辑选中 Actor 的标签信息
// Requirements: 3.2, 3.3, 5.1

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MAModifyWidget.generated.h"

class UMultiLineEditableTextBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UBorder;

/**
 * 确认修改委托
 * @param Actor 当前选中的 Actor
 * @param LabelText 文本框内容
 * Requirements: 5.1, 5.2
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModifyConfirmed, AActor*, Actor, const FString&, LabelText);

/**
 * Modify 模式修改面板 Widget - 纯 C++ 实现
 * 
 * 功能:
 * - 显示选中 Actor 的标签信息
 * - 提供可编辑的多行文本框
 * - 确认按钮提交修改
 * 
 * Requirements: 3.2, 3.3, 5.1
 */
UCLASS()
class MULTIAGENT_API UMAModifyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAModifyWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 设置选中的 Actor
     * @param Actor 选中的 Actor，nullptr 表示清除选择
     * Requirements: 4.1
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetSelectedActor(AActor* Actor);

    /**
     * 清除选中状态
     * 显示提示文字 "点击场景中的 Actor 进行选择"
     * Requirements: 4.3
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ClearSelection();

    /**
     * 获取文本框内容
     * @return 当前文本框中的文本
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    FString GetLabelText() const;

    /**
     * 设置文本框内容
     * @param Text 要设置的文本
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetLabelText(const FString& Text);

    /**
     * 获取当前选中的 Actor
     * @return 当前选中的 Actor，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    AActor* GetSelectedActor() const { return SelectedActor; }

    //=========================================================================
    // 事件委托
    //=========================================================================

    /**
     * 确认修改委托
     * 当用户点击确认按钮时广播
     * Requirements: 5.1, 5.2
     */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModifyConfirmed OnModifyConfirmed;

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // UI 控件 (动态创建)
    //=========================================================================

    /** 多行可编辑文本框 - 显示和编辑 Actor 标签 */
    UPROPERTY()
    UMultiLineEditableTextBox* LabelTextBox;

    /** 确认按钮 */
    UPROPERTY()
    UButton* ConfirmButton;

    /** 提示文本 */
    UPROPERTY()
    UTextBlock* HintText;

    /** 标题文本 */
    UPROPERTY()
    UTextBlock* TitleText;

private:
    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 当前选中的 Actor */
    UPROPERTY()
    AActor* SelectedActor = nullptr;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 确认按钮点击处理
     * Requirements: 5.1, 5.2, 5.3, 5.4
     */
    UFUNCTION()
    void OnConfirmButtonClicked();

    /**
     * 构建 UI 布局
     * Requirements: 3.2, 3.3
     */
    void BuildUI();

    /**
     * 生成 Actor 标签文本（占位符实现）
     * @param Actor 要生成标签的 Actor
     * @return 格式化的标签文本 "Actor: [ActorName]\nLabel: [placeholder]"
     * Requirements: 4.2
     */
    FString GenerateActorLabel(AActor* Actor) const;
};
