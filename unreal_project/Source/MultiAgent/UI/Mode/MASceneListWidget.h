// MASceneListWidget.h
// 场景列表面板 Widget - 显示 Goal 和 Zone 列表
// 
// 用于在 Edit Mode 中显示当前场景的 Goal 列表和 Zone 列表
// 支持点击列表项选中对应的 Actor

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MASceneListWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UButton;
class UScrollBox;
class UBorder;
class UMAEditModeManager;

//=============================================================================
// 委托声明
//=============================================================================

/** Goal 项点击委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoalItemClicked, const FString&, GoalId);

/** Zone 项点击委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZoneItemClicked, const FString&, ZoneId);

/**
 * 场景列表面板 Widget
 * 
 * 显示当前场景的 Goal 列表和 Zone 列表
 */
UCLASS()
class MULTIAGENT_API UMASceneListWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMASceneListWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================
    
    /**
     * 刷新列表内容
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void RefreshLists();

    /**
     * 设置 EditModeManager 引用
     * @param InManager EditModeManager 指针
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetEditModeManager(UMAEditModeManager* InManager);

    //=========================================================================
    // 委托
    //=========================================================================
    
    /** Goal 项点击委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnGoalItemClicked OnGoalItemClicked;

    /** Zone 项点击委托 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnZoneItemClicked OnZoneItemClicked;

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
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // 主题引用
    //=========================================================================

    /** 缓存的主题引用 */
    UPROPERTY()
    class UMAUITheme* Theme;

private:
    //=========================================================================
    // UI 控件
    //=========================================================================
    
    /** Goal 列表容器 */
    UPROPERTY()
    UVerticalBox* GoalListBox;

    /** Zone 列表容器 */
    UPROPERTY()
    UVerticalBox* ZoneListBox;

    /** Goal 标题 */
    UPROPERTY()
    UTextBlock* GoalTitleText;

    /** Zone 标题 */
    UPROPERTY()
    UTextBlock* ZoneTitleText;

    /** Goal 计数文本 */
    UPROPERTY()
    UTextBlock* GoalCountText;

    /** Zone 计数文本 */
    UPROPERTY()
    UTextBlock* ZoneCountText;

    //=========================================================================
    // 内部状态
    //=========================================================================
    
    /** EditModeManager 引用 */
    UPROPERTY()
    UMAEditModeManager* EditModeManager;

    /** Goal 按钮数组 */
    UPROPERTY()
    TArray<UButton*> GoalButtons;

    /** Zone 按钮数组 */
    UPROPERTY()
    TArray<UButton*> ZoneButtons;

    /** Goal ID 映射 */
    TArray<FString> GoalIds;

    /** Zone ID 映射 */
    TArray<FString> ZoneIds;

    //=========================================================================
    // 内部方法
    //=========================================================================
    
    /** 构建 UI */
    void BuildUI();

    /** 填充 Goal 列表 */
    void PopulateGoalList();

    /** 填充 Zone 列表 */
    void PopulateZoneList();

    /** Goal 按钮点击处理 */
    UFUNCTION()
    void OnGoalButtonClicked();

    /** Zone 按钮点击处理 */
    UFUNCTION()
    void OnZoneButtonClicked();

    /** 创建列表项按钮 */
    UButton* CreateListItemButton(const FString& Label, const FString& Id, bool bIsGoal);
};
