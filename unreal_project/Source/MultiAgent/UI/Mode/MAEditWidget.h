// MAEditWidget.h
// Edit Mode 编辑面板 Widget - 纯 C++ 实现
// 用于 Edit Mode 下查看和编辑选中 Actor/POI 的信息
// 支持 Actor 属性编辑、Goal 创建、Zone 创建

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Domain/MAEditWidgetModel.h"
#include "MAEditWidget.generated.h"

class UMultiLineEditableTextBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class UScrollBox;
class UComboBoxString;
class AMAPointOfInterest;

//=============================================================================
// 委托声明
//=============================================================================

/**
 * 确认修改委托
 * @param Actor 当前选中的 Actor
 * @param JsonContent JSON 编辑内容
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEditConfirmRequested, const FString&, JsonContent);

/**
 * 删除 Actor 委托
 * @param Actor 要删除的 Actor
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeleteActorRequested);

/**
 * 创建 Goal 委托
 * @param POI 用于创建 Goal 的 POI
 * @param Description Goal 描述
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateGoalRequested, const FString&, Description);

/**
 * 创建 Zone 委托
 * @param POIs 用于创建 Zone 的 POI 数组
 * @param Description Zone 描述
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCreateZoneRequested, const FString&, Description);

/**
 * 添加预设 Actor 委托
 * @param POI 目标 POI
 * @param ActorType 预设 Actor 类型
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddPresetActorRequested, const FString&, ActorType);

/**
 * 删除 POI 委托
 * @param POIs 要删除的 POI 数组
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeletePOIsRequested);

/**
 * 设为 Goal 委托
 * @param Actor 目标 Actor
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSetAsGoalRequested);

/**
 * 取消 Goal 委托
 * @param Actor 目标 Actor
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnsetAsGoalRequested);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEditNodeSwitchRequested, int32, NodeIndex);

/**
 * Edit Mode 编辑面板 Widget - 纯 C++ 实现
 * 
 * 功能:
 * - 显示选中 Actor 的 JSON 信息
 * - 支持 JSON 编辑和验证
 * - 支持 Actor 删除
 * - 支持 Goal 创建 (单选 POI)
 * - 支持 Zone 创建 (多选 POI >= 3)
 * - 支持预设 Actor 添加
 * - 临时场景图预览
 * 
 */
UCLASS()
class MULTIAGENT_API UMAEditWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAEditWidget(const FObjectInitializer& ObjectInitializer);

    void ApplyViewModel(const FMAEditWidgetViewModel& InViewModel);
    void SetPresetActorOptions(const TArray<FString>& Options);

    //=========================================================================
    // 事件委托
    //=========================================================================

    /** 确认修改请求 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEditConfirmRequested OnEditConfirmRequested;

    /** 删除 Actor 请求 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDeleteActorRequested OnDeleteActorRequested;

    /** 创建 Goal 请求 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCreateGoalRequested OnCreateGoalRequested;

    /** 创建 Zone 请求 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCreateZoneRequested OnCreateZoneRequested;

    /** 添加预设 Actor 请求 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnAddPresetActorRequested OnAddPresetActorRequested;

    /** 删除 POI 请求 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDeletePOIsRequested OnDeletePOIsRequested;

    /** 设为 Goal 请求 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSetAsGoalRequested OnSetAsGoalRequested;

    /** 取消 Goal 请求 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnUnsetAsGoalRequested OnUnsetAsGoalRequested;

    /** Node 切换请求 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEditNodeSwitchRequested OnNodeSwitchRequested;

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

    /** JSON 编辑文本框 - 显示和编辑 Actor 对应的 JSON */
    UPROPERTY()
    UMultiLineEditableTextBox* JsonEditBox;

    /** 描述输入文本框 - 用于 Goal/Zone 描述 */
    UPROPERTY()
    UMultiLineEditableTextBox* DescriptionBox;

    /** 场景图预览文本 - 只读，显示临时场景图内容 */
    UPROPERTY()
    UTextBlock* SceneGraphPreviewText;

    /** 确认按钮 */
    UPROPERTY()
    UButton* ConfirmButton;

    /** 删除按钮 */
    UPROPERTY()
    UButton* DeleteButton;

    /** 创建 Goal 按钮 */
    UPROPERTY()
    UButton* CreateGoalButton;

    /** 创建 Zone 按钮 */
    UPROPERTY()
    UButton* CreateZoneButton;

    /** 预设 Actor 下拉框 */
    UPROPERTY()
    UComboBoxString* PresetActorComboBox;

    /** 添加 Actor 按钮 */
    UPROPERTY()
    UButton* AddActorButton;

    /** 删除 POI 按钮 */
    UPROPERTY()
    UButton* DeletePOIButton;

    /** 设为 Goal 按钮 */
    UPROPERTY()
    UButton* SetAsGoalButton;

    /** 取消 Goal 按钮 */
    UPROPERTY()
    UButton* UnsetAsGoalButton;

    /** Node 切换按钮容器 */
    UPROPERTY()
    UHorizontalBox* NodeSwitchContainer;

    /** 提示文本 */
    UPROPERTY()
    UTextBlock* HintText;

    /** 标题文本 */
    UPROPERTY()
    UTextBlock* TitleText;

    /** 错误提示文本 */
    UPROPERTY()
    UTextBlock* ErrorText;

    /** Actor 操作区域容器 */
    UPROPERTY()
    UVerticalBox* ActorOperationBox;

    /** POI 操作区域容器 */
    UPROPERTY()
    UVerticalBox* POIOperationBox;

    /** 场景图预览滚动框 */
    UPROPERTY()
    UScrollBox* SceneGraphScrollBox;

private:
    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 构建 UI 布局
     */
    void BuildUI();

    /**
     * 更新 UI 状态
     * 根据当前选择状态显示/隐藏相应的 UI 元素
     */
    void UpdateUIState();

    /**
     * 更新 JSON 编辑框内容
     */
    void UpdateJsonEditBox();

    /**
     * 更新 Node 切换按钮
     */
    void UpdateNodeSwitchButtons();

    /**
     * 显示错误信息
     * @param ErrorMessage 错误信息
     */
    void ShowError(const FString& ErrorMessage);

    /**
     * 清除错误信息
     */
    void ClearError();

    FString GetDescriptionText() const;
    FString GetJsonEditContent() const;
    FLinearColor ResolveHintColor(EMAEditWidgetHintTone Tone) const;

    //=========================================================================
    // 按钮点击处理
    //=========================================================================

    /** 确认按钮点击 */
    UFUNCTION()
    void OnConfirmButtonClicked();

    /** 删除按钮点击 */
    UFUNCTION()
    void OnDeleteButtonClicked();

    /** 创建 Goal 按钮点击 */
    UFUNCTION()
    void OnCreateGoalButtonClicked();

    /** 创建 Zone 按钮点击 */
    UFUNCTION()
    void OnCreateZoneButtonClicked();

    /** 添加 Actor 按钮点击 */
    UFUNCTION()
    void OnAddActorButtonClicked();

    /** 删除 POI 按钮点击 */
    UFUNCTION()
    void OnDeletePOIButtonClicked();

    /** 设为 Goal 按钮点击 */
    UFUNCTION()
    void OnSetAsGoalButtonClicked();

    /** 取消 Goal 按钮点击 */
    UFUNCTION()
    void OnUnsetAsGoalButtonClicked();

    /** Node 切换按钮点击 */
    void OnNodeSwitchButtonClicked(int32 NodeIndex);
    
    /** Node 切换按钮点击内部处理 (用于动态委托) */
    UFUNCTION()
    void OnNodeSwitchButtonClickedInternal();
    
    /** Node 切换按钮数组 (用于查找点击的按钮索引) */
    UPROPERTY()
    TArray<UButton*> NodeSwitchButtons;

    FMAEditWidgetViewModel ViewModel;
};
