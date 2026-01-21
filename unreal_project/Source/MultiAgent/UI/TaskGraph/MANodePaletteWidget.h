// MANodePaletteWidget.h
// 节点工具栏 Widget - 提供可拖拽的节点模板

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "MANodePaletteWidget.generated.h"

class UVerticalBox;
class UScrollBox;
class UBorder;
class UTextBlock;
class UButton;
class UMAUITheme;

//=============================================================================
// 委托声明
//=============================================================================

/** 节点模板选中委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeTemplateSelected, const FMANodeTemplate&, Template);

/** 节点模板拖拽开始委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeTemplateDragStarted, const FMANodeTemplate&, Template, FVector2D, StartPosition);

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMANodePalette, Log, All);

//=============================================================================
// UMANodePaletteWidget - 节点工具栏 Widget
//=============================================================================

/**
 * 节点工具栏 Widget
 * 
 * 提供预定义的节点模板列表，用户可以点击或拖拽模板到画布创建新节点
 * 
 * 布局结构:
 * ┌─────────────────────────┐
 * │  节点模板               │
 * ├─────────────────────────┤
 * │  [📍 Navigate]          │
 * │  [🔄 Patrol]            │
 * │  [👁 Perceive]          │
 * │  [📢 Broadcast]         │
 * │  [⚙ Custom]             │
 * └─────────────────────────┘
 */
UCLASS()
class MULTIAGENT_API UMANodePaletteWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMANodePaletteWidget(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 初始化
    //=========================================================================

    /** 初始化节点模板列表 */
    UFUNCTION(BlueprintCallable, Category = "NodePalette")
    void InitializeTemplates();

    /** 添加自定义模板 */
    UFUNCTION(BlueprintCallable, Category = "NodePalette")
    void AddTemplate(const FMANodeTemplate& Template);

    /** 清空所有模板 */
    UFUNCTION(BlueprintCallable, Category = "NodePalette")
    void ClearTemplates();

    //=========================================================================
    // 模板访问
    //=========================================================================

    /** 获取所有模板 */
    UFUNCTION(BlueprintPure, Category = "NodePalette")
    TArray<FMANodeTemplate> GetTemplates() const { return NodeTemplates; }

    /** 根据名称获取模板 */
    UFUNCTION(BlueprintPure, Category = "NodePalette")
    FMANodeTemplate GetTemplateByName(const FString& TemplateName) const;

    //=========================================================================
    // 委托
    //=========================================================================

    /** 节点模板选中委托 (点击) */
    UPROPERTY(BlueprintAssignable, Category = "NodePalette|Events")
    FOnNodeTemplateSelected OnNodeTemplateSelected;

    /** 节点模板拖拽开始委托 */
    UPROPERTY(BlueprintAssignable, Category = "NodePalette|Events")
    FOnNodeTemplateDragStarted OnNodeTemplateDragStarted;

    //=========================================================================
    // 主题
    //=========================================================================

    /** 应用主题样式 */
    UFUNCTION(BlueprintCallable, Category = "NodePalette")
    void ApplyTheme(UMAUITheme* InTheme);

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // UI 构建
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 刷新模板列表显示 */
    void RefreshTemplateList();

    /** 创建单个模板按钮 */
    UButton* CreateTemplateButton(const FMANodeTemplate& Template, int32 Index);

    //=========================================================================
    // 事件处理
    //=========================================================================

    /** 模板按钮点击回调 - 使用按钮索引查找 */
    UFUNCTION()
    void OnTemplateButton0Clicked();
    UFUNCTION()
    void OnTemplateButton1Clicked();
    UFUNCTION()
    void OnTemplateButton2Clicked();
    UFUNCTION()
    void OnTemplateButton3Clicked();
    UFUNCTION()
    void OnTemplateButton4Clicked();

    /** 通用模板点击处理 */
    void HandleTemplateClicked(int32 TemplateIndex);

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 背景边框 */
    UPROPERTY()
    UBorder* BackgroundBorder;

    /** 标题文本 */
    UPROPERTY()
    UTextBlock* TitleText;

    /** 模板列表容器 */
    UPROPERTY()
    UScrollBox* TemplateScrollBox;

    /** 模板按钮容器 */
    UPROPERTY()
    UVerticalBox* TemplateListBox;

    /** 模板按钮数组 */
    UPROPERTY()
    TArray<UButton*> TemplateButtons;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 节点模板列表 */
    UPROPERTY()
    TArray<FMANodeTemplate> NodeTemplates;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 工具栏宽度 */
    float PaletteWidth = 180.0f;

    /** 按钮高度 */
    float ButtonHeight = 40.0f;

    /** 按钮间距 */
    float ButtonSpacing = 5.0f;

    //=========================================================================
    // 主题引用
    //=========================================================================

    /** 缓存的主题引用 */
    UPROPERTY()
    UMAUITheme* Theme;

    //=========================================================================
    // 颜色配置 (从 Theme 获取，带 fallback 默认值)
    //=========================================================================

    /** 背景颜色 */
    FLinearColor BackgroundColor = FLinearColor(0.1f, 0.1f, 0.12f, 0.95f);

    /** 标题颜色 */
    FLinearColor TitleColor = FLinearColor(0.8f, 0.8f, 0.9f, 1.0f);

    /** 按钮默认颜色 */
    FLinearColor ButtonDefaultColor = FLinearColor(0.2f, 0.2f, 0.25f, 1.0f);

    /** 按钮悬浮颜色 */
    FLinearColor ButtonHoverColor = FLinearColor(0.3f, 0.3f, 0.4f, 1.0f);

    /** 按钮文本颜色 */
    FLinearColor ButtonTextColor = FLinearColor(0.9f, 0.9f, 1.0f, 1.0f);

private:
    /** 是否已初始化默认模板 */
    bool bTemplatesInitialized = false;
};
