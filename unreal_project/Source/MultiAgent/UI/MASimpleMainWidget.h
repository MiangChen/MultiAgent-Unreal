// MASimpleMainWidget.h
// 简单的主界面 Widget - 纯 C++ 实现，不需要蓝图
// 用于快速测试 UI 功能

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MASimpleMainWidget.generated.h"

class UEditableTextBox;
class UButton;
class UMultiLineEditableTextBox;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;

/**
 * 指令提交委托
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimpleCommandSubmitted, const FString&, Command);

/**
 * 简单的主界面 Widget - 纯 C++ 实现
 * 
 * 在 C++ 中动态创建所有 UI 控件，不需要蓝图
 */
UCLASS()
class MULTIAGENT_API UMASimpleMainWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMASimpleMainWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================

    /** 设置结果显示文本 */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetResultText(const FString& Text);

    /** 将焦点设置到输入框 */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void FocusInputBox();

    /** 清空输入框内容 */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ClearInputBox();

    /** 获取输入框当前文本 */
    UFUNCTION(BlueprintPure, Category = "UI")
    FString GetInputText() const;

    //=========================================================================
    // 事件委托
    //=========================================================================

    /** 指令提交委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSimpleCommandSubmitted OnCommandSubmitted;

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // UI 控件 (动态创建)
    //=========================================================================

    UPROPERTY()
    UEditableTextBox* InputTextBox;

    UPROPERTY()
    UButton* SendButton;

    UPROPERTY()
    UMultiLineEditableTextBox* ResultTextBox;

private:
    /** 发送按钮点击处理 */
    UFUNCTION()
    void OnSendButtonClicked();

    /** 输入框回车提交处理 */
    UFUNCTION()
    void OnInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    /** 提交指令 */
    void SubmitCommand();

    /** 构建 UI 布局 */
    void BuildUI();
};
