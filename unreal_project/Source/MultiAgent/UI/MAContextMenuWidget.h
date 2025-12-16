// MAContextMenuWidget.h
// 上下文菜单 Widget - 用于节点和边的右键菜单
// Requirements: 6.4, 7.3

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MAContextMenuWidget.generated.h"

class UVerticalBox;
class UButton;
class UTextBlock;
class UBorder;

//=============================================================================
// 菜单项数据
//=============================================================================

USTRUCT(BlueprintType)
struct FMAContextMenuItem
{
    GENERATED_BODY()

    /** 菜单项 ID */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemId;

    /** 显示文本 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayText;

    /** 是否启用 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnabled = true;

    FMAContextMenuItem() {}
    FMAContextMenuItem(const FString& InId, const FString& InText, bool bInEnabled = true)
        : ItemId(InId), DisplayText(InText), bEnabled(bInEnabled) {}
};

//=============================================================================
// 委托声明
//=============================================================================

/** 菜单项点击委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnContextMenuItemClicked, const FString&, ItemId);

/** 菜单关闭委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnContextMenuClosed);

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMAContextMenu, Log, All);

//=============================================================================
// UMAContextMenuWidget - 上下文菜单 Widget
//=============================================================================

/**
 * 上下文菜单 Widget
 * 
 * 用于显示节点和边的右键上下文菜单
 */
UCLASS()
class MULTIAGENT_API UMAContextMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAContextMenuWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 菜单操作
    //=========================================================================

    /** 设置菜单项 */
    UFUNCTION(BlueprintCallable, Category = "ContextMenu")
    void SetMenuItems(const TArray<FMAContextMenuItem>& Items);

    /** 显示菜单 */
    UFUNCTION(BlueprintCallable, Category = "ContextMenu")
    void ShowAtPosition(FVector2D ScreenPosition);

    /** 关闭菜单 */
    UFUNCTION(BlueprintCallable, Category = "ContextMenu")
    void CloseMenu();

    //=========================================================================
    // 委托
    //=========================================================================

    /** 菜单项点击委托 */
    UPROPERTY(BlueprintAssignable, Category = "ContextMenu|Events")
    FOnContextMenuItemClicked OnItemClicked;

    /** 菜单关闭委托 */
    UPROPERTY(BlueprintAssignable, Category = "ContextMenu|Events")
    FOnContextMenuClosed OnMenuClosed;

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    //=========================================================================
    // UI 构建
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 重建菜单项 */
    void RebuildMenuItems();

    /** 创建菜单项按钮 */
    UButton* CreateMenuItemButton(const FMAContextMenuItem& Item);

    /** 菜单项点击处理 */
    void OnMenuItemButtonClicked(const FString& ItemId);

    /** 菜单项点击回调 (按索引) */
    UFUNCTION()
    void OnItem0Clicked();
    UFUNCTION()
    void OnItem1Clicked();
    UFUNCTION()
    void OnItem2Clicked();
    UFUNCTION()
    void OnItem3Clicked();

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 菜单背景 */
    UPROPERTY()
    UBorder* MenuBackground;

    /** 菜单项容器 */
    UPROPERTY()
    UVerticalBox* ItemContainer;

    /** 菜单项按钮列表 */
    UPROPERTY()
    TArray<UButton*> ItemButtons;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 菜单项列表 */
    TArray<FMAContextMenuItem> MenuItems;

    //=========================================================================
    // 颜色配置
    //=========================================================================

    /** 菜单背景颜色 */
    FLinearColor MenuBackgroundColor = FLinearColor(0.15f, 0.15f, 0.2f, 0.95f);

    /** 菜单项默认颜色 */
    FLinearColor ItemDefaultColor = FLinearColor(0.2f, 0.2f, 0.25f, 1.0f);

    /** 菜单项悬浮颜色 */
    FLinearColor ItemHoverColor = FLinearColor(0.3f, 0.3f, 0.4f, 1.0f);

    /** 菜单项禁用颜色 */
    FLinearColor ItemDisabledColor = FLinearColor(0.15f, 0.15f, 0.15f, 0.5f);

    /** 文本颜色 */
    FLinearColor TextColor = FLinearColor(0.9f, 0.9f, 0.95f, 1.0f);

    /** 禁用文本颜色 */
    FLinearColor DisabledTextColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);
};
