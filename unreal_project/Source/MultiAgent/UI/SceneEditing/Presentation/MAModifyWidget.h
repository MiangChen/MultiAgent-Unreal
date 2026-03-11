// MAModifyWidget.h
// Modify 模式修改面板 Widget - 纯 C++ 实现
// 用于查看和编辑选中 Actor 的标签信息
// 支持单选和多选模式

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Application/MAModifyWidgetCoordinator.h"
#include "../Domain/MAModifyWidgetModel.h"
#include "../MAModifyTypes.h"
#include "MAModifyWidget.generated.h"

class UMultiLineEditableTextBox;
class UMultiLineEditableText;
class UButton;
class UTextBlock;
class UVerticalBox;
class UBorder;
class UScrollBox;
struct FMASceneGraphNode;

/**
 * 确认修改委托 (单选模式)
 * @param Actor 当前选中的 Actor
 * @param LabelText 文本框内容
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModifyConfirmed, AActor*, Actor, const FString&, LabelText);

/**
 * 确认修改委托 (多选模式)
 * @param Actors 当前选中的 Actor 数组
 * @param LabelText 文本框内容
 * @param GeneratedJson 生成的 JSON 字符串
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMultiSelectModifyConfirmed, const TArray<AActor*>&, Actors, const FString&, LabelText, const FString&, GeneratedJson);

/**
 * Modify 模式修改面板 Widget - 纯 C++ 实现
 * 
 * 功能:
 * - 显示选中 Actor 的标签信息
 * - 提供可编辑的多行文本框
 * - 确认按钮提交修改
 * - 支持多选模式 (Shift+Click)
 * 
 */
UCLASS()
class MULTIAGENT_API UMAModifyWidget : public UUserWidget
{
    GENERATED_BODY()

    friend class FMAModifyWidgetCoordinator;

public:
    UMAModifyWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口 - 单选模式 (向后兼容)
    //=========================================================================

    /**
     * 设置选中的 Actor (单选模式)
     * @param Actor 选中的 Actor，nullptr 表示清除选择
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetSelectedActor(AActor* Actor);

    /**
     * 清除选中状态
     * 显示提示文字 "点击场景中的 Actor 进行选择"
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
     * 获取当前选中的 Actor (单选模式)
     * @return 当前选中的 Actor，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    AActor* GetSelectedActor() const { return SelectedActors.Num() > 0 ? SelectedActors[0] : nullptr; }

    //=========================================================================
    // 公共接口 - 多选模式
    //=========================================================================

    /**
     * 设置多个选中的 Actor (多选模式)
     * @param Actors 选中的 Actor 数组
     */
    UFUNCTION(BlueprintCallable, Category = "UI|MultiSelect")
    void SetSelectedActors(const TArray<AActor*>& Actors);

    /**
     * 获取所有选中的 Actor
     * @return 选中的 Actor 数组
     */
    UFUNCTION(BlueprintPure, Category = "UI|MultiSelect")
    TArray<AActor*> GetSelectedActors() const { return SelectedActors; }

    /**
     * 是否处于多选模式
     * @return true 如果选中了多个 Actor
     */
    UFUNCTION(BlueprintPure, Category = "UI|MultiSelect")
    bool IsMultiSelectMode() const { return SelectedActors.Num() > 1; }

    /**
     * 获取选中的 Actor 数量
     * @return 选中的 Actor 数量
     */
    UFUNCTION(BlueprintPure, Category = "UI|MultiSelect")
    int32 GetSelectionCount() const { return SelectedActors.Num(); }

    //=========================================================================
    // 事件委托
    //=========================================================================

    /**
     * 确认修改委托 (单选模式)
     * 当用户点击确认按钮时广播
     */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModifyConfirmed OnModifyConfirmed;

    /**
     * 确认修改委托 (多选模式)
     * 当用户在多选模式下点击确认按钮时广播
     */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMultiSelectModifyConfirmed OnMultiSelectModifyConfirmed;

    //=========================================================================
    // 主题支持
    //=========================================================================

    /**
     * 应用主题样式
     * @param InTheme 要应用的主题
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Theme")
    void ApplyTheme(class UMAUITheme* InTheme);

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // 主题引用
    //=========================================================================

    /** 缓存的主题引用 */
    UPROPERTY()
    class UMAUITheme* Theme;

    //=========================================================================
    // UI 控件 (动态创建)
    //=========================================================================

    /** 多行可编辑文本框 - 显示和编辑 Actor 标签 */
    UPROPERTY()
    UMultiLineEditableTextBox* LabelTextBox;

    /** JSON 预览文本框 - 只读，显示 Actor 对应的 JSON 片段 */
    UPROPERTY()
    UTextBlock* JsonPreviewText;

    /** 确认按钮 */
    UPROPERTY()
    UButton* ConfirmButton;

    /** 提示文本 */
    UPROPERTY()
    UTextBlock* HintText;

    /** 标题文本 */
    UPROPERTY()
    UTextBlock* TitleText;

    /** 模式指示器文本 */
    UPROPERTY()
    UTextBlock* ModeIndicatorText;

    /** 当前标注模式 */
    UPROPERTY()
    EMAAnnotationMode CurrentAnnotationMode = EMAAnnotationMode::AddNew;

    /** 编辑模式下的节点 ID */
    UPROPERTY()
    FString EditingNodeId;

private:
    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 当前选中的 Actor 数组 (支持多选) */
    UPROPERTY()
    TArray<AActor*> SelectedActors;
    FMAModifyWidgetCoordinator Coordinator;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 确认按钮点击处理
     */
    UFUNCTION()
    void OnConfirmButtonClicked();

    /**
     * 构建 UI 布局
     */
    void BuildUI();

    void ApplySelectionViewModel(const FMAModifySelectionViewModel& ViewModel);
    void ApplyPreviewModel(const FMAModifyPreviewModel& PreviewModel);

    /**
     * 设置标注模式
     * @param Mode 新模式
     * @param NodeId 编辑模式下的节点 ID
     */
    void SetAnnotationMode(EMAAnnotationMode Mode, const FString& NodeId);

    /**
     * 更新模式指示器 UI
     */
    void UpdateModeIndicator();

    FMAModifyWidgetSelectionModels RuntimeBuildSingleSelectionModels(AActor* Actor) const;
    FMAModifyWidgetSelectionModels RuntimeBuildMultiSelectionModels(const TArray<AActor*>& Actors) const;
    FMAModifyWidgetSubmitResult RuntimeBuildSubmitResult(
        const TArray<AActor*>& Actors,
        const FString& LabelText,
        EMAAnnotationMode AnnotationMode,
        const FString& NodeId) const;

};
