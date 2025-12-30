// MAEditWidget.h
// Edit Mode 编辑面板 Widget - 纯 C++ 实现
// 用于 Edit Mode 下查看和编辑选中 Actor/POI 的信息
// 支持 Actor 属性编辑、Goal 创建、Zone 创建
// Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 6.1, 6.2, 6.3, 6.4, 7.1, 8.1, 8.2, 8.5, 9.1, 9.2, 9.6, 10.1, 10.2, 10.7, 13.1, 13.2, 13.3, 13.4

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Core/MASceneGraphManager.h"  // For FMASceneGraphNode
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
 * Requirements: 6.4
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEditConfirmed, AActor*, Actor, const FString&, JsonContent);

/**
 * 删除 Actor 委托
 * @param Actor 要删除的 Actor
 * Requirements: 7.1
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeleteActor, AActor*, Actor);

/**
 * 创建 Goal 委托
 * @param POI 用于创建 Goal 的 POI
 * @param Description Goal 描述
 * Requirements: 9.6
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCreateGoal, AMAPointOfInterest*, POI, const FString&, Description);

/**
 * 创建 Zone 委托
 * @param POIs 用于创建 Zone 的 POI 数组
 * @param Description Zone 描述
 * Requirements: 10.7
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCreateZone, const TArray<AMAPointOfInterest*>&, POIs, const FString&, Description);

/**
 * 添加预设 Actor 委托
 * @param POI 目标 POI
 * @param ActorType 预设 Actor 类型
 * Requirements: 8.2
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAddPresetActor, AMAPointOfInterest*, POI, const FString&, ActorType);

/**
 * 设为 Goal 委托
 * @param Actor 目标 Actor
 * Requirements: 16.2
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSetAsGoal, AActor*, Actor);

/**
 * 取消 Goal 委托
 * @param Actor 目标 Actor
 * Requirements: 16.6
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnsetAsGoal, AActor*, Actor);

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
 * Requirements: 5.1, 5.6
 */
UCLASS()
class MULTIAGENT_API UMAEditWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAEditWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口 - Actor 选择
    // Requirements: 5.1, 5.2, 5.3, 5.4, 5.5
    //=========================================================================

    /**
     * 设置选中的 Actor
     * @param Actor 选中的 Actor，nullptr 表示清除选择
     * Requirements: 5.1
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetSelectedActor(AActor* Actor);

    /**
     * 获取当前选中的 Actor
     * @return 当前选中的 Actor，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    AActor* GetSelectedActor() const { return CurrentActor; }

    //=========================================================================
    // 公共接口 - POI 选择
    // Requirements: 8.1, 9.1, 10.1
    //=========================================================================

    /**
     * 设置选中的 POI 列表
     * @param POIs 选中的 POI 数组
     * Requirements: 8.1, 9.1, 10.1
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetSelectedPOIs(const TArray<AMAPointOfInterest*>& POIs);

    /**
     * 获取选中的 POI 列表
     * @return 选中的 POI 数组
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    TArray<AMAPointOfInterest*> GetSelectedPOIs() const { return CurrentPOIs; }

    //=========================================================================
    // 公共接口 - 通用
    //=========================================================================

    /**
     * 清除选择
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ClearSelection();

    /**
     * 刷新临时场景图预览
     * Requirements: 13.2
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void RefreshSceneGraphPreview();

    /**
     * 获取描述文本
     * @return 描述输入框内容
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    FString GetDescriptionText() const;

    /**
     * 获取 JSON 编辑内容
     * @return JSON 编辑框内容
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    FString GetJsonEditContent() const;

    //=========================================================================
    // 事件委托
    //=========================================================================

    /** 确认修改委托 - Requirements: 6.4 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEditConfirmed OnEditConfirmed;

    /** 删除 Actor 委托 - Requirements: 7.1 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDeleteActor OnDeleteActor;

    /** 创建 Goal 委托 - Requirements: 9.6 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCreateGoal OnCreateGoal;

    /** 创建 Zone 委托 - Requirements: 10.7 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnCreateZone OnCreateZone;

    /** 添加预设 Actor 委托 - Requirements: 8.2 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnAddPresetActor OnAddPresetActor;

    /** 设为 Goal 委托 - Requirements: 16.2 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSetAsGoal OnSetAsGoal;

    /** 取消 Goal 委托 - Requirements: 16.6 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnUnsetAsGoal OnUnsetAsGoal;

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

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

    /** 设为 Goal 按钮 - Requirements: 16.1 */
    UPROPERTY()
    UButton* SetAsGoalButton;

    /** 取消 Goal 按钮 - Requirements: 16.5 */
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
    // 内部状态
    //=========================================================================

    /** 当前选中的 Actor */
    UPROPERTY()
    AActor* CurrentActor = nullptr;

    /** 当前选中的 POI 列表 */
    UPROPERTY()
    TArray<AMAPointOfInterest*> CurrentPOIs;

    /** 当前显示的 Node 索引 (用于多 Node 切换) */
    int32 CurrentNodeIndex = 0;

    /** Actor 对应的所有 Node */
    TArray<FMASceneGraphNode> ActorNodes;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 构建 UI 布局
     * Requirements: 5.1, 5.6, 8.1, 9.1, 10.1
     */
    void BuildUI();

    /**
     * 更新 UI 状态
     * 根据当前选择状态显示/隐藏相应的 UI 元素
     */
    void UpdateUIState();

    /**
     * 更新 JSON 编辑框内容
     * Requirements: 5.1, 5.2
     */
    void UpdateJsonEditBox();

    /**
     * 更新 Node 切换按钮
     * Requirements: 5.2
     */
    void UpdateNodeSwitchButtons();

    /**
     * 验证 JSON 格式
     * @param Json JSON 字符串
     * @param OutError 错误信息
     * @return 是否有效
     * Requirements: 6.1, 6.2
     */
    bool ValidateJson(const FString& Json, FString& OutError);

    /**
     * 检查是否为 point 类型 Node
     * @param Node 场景图节点
     * @return 是否为 point 类型
     * Requirements: 5.3, 5.5
     */
    bool IsPointTypeNode(const FMASceneGraphNode& Node) const;

    /**
     * 显示错误信息
     * @param ErrorMessage 错误信息
     */
    void ShowError(const FString& ErrorMessage);

    /**
     * 清除错误信息
     */
    void ClearError();

    /**
     * 高亮显示选中 Node 的 JSON
     * @param NodeId 要高亮的 Node ID
     */
    void HighlightNodeInPreview(const FString& NodeId);

    //=========================================================================
    // 按钮点击处理
    //=========================================================================

    /** 确认按钮点击 - Requirements: 6.4 */
    UFUNCTION()
    void OnConfirmButtonClicked();

    /** 删除按钮点击 - Requirements: 7.1 */
    UFUNCTION()
    void OnDeleteButtonClicked();

    /** 创建 Goal 按钮点击 - Requirements: 9.6 */
    UFUNCTION()
    void OnCreateGoalButtonClicked();

    /** 创建 Zone 按钮点击 - Requirements: 10.7 */
    UFUNCTION()
    void OnCreateZoneButtonClicked();

    /** 添加 Actor 按钮点击 - Requirements: 8.2 */
    UFUNCTION()
    void OnAddActorButtonClicked();

    /** 设为 Goal 按钮点击 - Requirements: 16.2 */
    UFUNCTION()
    void OnSetAsGoalButtonClicked();

    /** 取消 Goal 按钮点击 - Requirements: 16.6 */
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
};
