// MAPreviewPanel.h
// 预览面板 Widget - 包含任务图预览和技能列表预览
// 从 MARightSidebarWidget 拆分出来，可通过按键独立切换显示/隐藏

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Shared/Types/MATaskGraphTypes.h"
#include "MAPreviewPanel.generated.h"

class UVerticalBox;
class UBorder;
class UTextBlock;
class USizeBox;
class UMAUITheme;
class UMATaskGraphPreview;
class UMASkillListPreview;

// 日志类别声明
DECLARE_LOG_CATEGORY_EXTERN(LogMAPreviewPanel, Log, All);

/**
 * 预览面板 Widget
 * 
 * 独立的预览显示面板，从 MARightSidebarWidget 拆分而来。
 * 可通过按键 7 独立切换显示/隐藏状态。
 * 
 * 功能：
 * - 显示任务图预览 (Task Graph Preview)
 * - 显示技能列表预览 (Skill List Preview)
 * - 支持预览数据更新
 * - 支持主题切换
 * 
 */
UCLASS()
class MULTIAGENT_API UMAPreviewPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    UMAPreviewPanel(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 预览更新方法
    //=========================================================================

    /**
     * 更新任务图预览
     * @param Data 任务图数据
     */
    UFUNCTION(BlueprintCallable, Category = "Panel|Preview")
    void UpdateTaskGraphPreview(const FMATaskGraphData& Data);

    /**
     * 更新技能列表预览
     * @param Data 技能分配数据
     */
    UFUNCTION(BlueprintCallable, Category = "Panel|Preview")
    void UpdateSkillListPreview(const FMASkillAllocationData& Data);

    /**
     * 更新单个技能的执行状态
     * 用于执行过程中实时更新显示
     * @param TimeStep 时间步
     * @param RobotId 机器人 ID
     * @param NewStatus 新状态
     */
    UFUNCTION(BlueprintCallable, Category = "Panel|Preview")
    void UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus);

    //=========================================================================
    // 组件访问方法
    //=========================================================================

    /**
     * 获取任务图预览组件
     * @return 任务图预览组件
     */
    UFUNCTION(BlueprintPure, Category = "Panel|Components")
    UMATaskGraphPreview* GetTaskGraphPreview() const { return TaskGraphPreview; }

    /**
     * 获取技能列表预览组件
     * @return 技能列表预览组件
     */
    UFUNCTION(BlueprintPure, Category = "Panel|Components")
    UMASkillListPreview* GetSkillListPreview() const { return SkillListPreview; }

    //=========================================================================
    // 主题支持
    //=========================================================================

    /**
     * 应用主题
     * @param InTheme 主题数据资产
     */
    UFUNCTION(BlueprintCallable, Category = "Theme")
    void ApplyTheme(UMAUITheme* InTheme);

    /**
     * 获取当前主题
     * @return 当前主题
     */
    UFUNCTION(BlueprintPure, Category = "Theme")
    UMAUITheme* GetTheme() const { return Theme; }

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    //=========================================================================
    // 配置属性
    //=========================================================================

    /** 面板宽度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "200.0", ClampMax = "800.0"))
    float PanelWidth = 480.0f;

    /** 主题引用 */
    UPROPERTY(EditDefaultsOnly, Category = "Theme")
    UMAUITheme* Theme;

private:
    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 根 SizeBox - 控制面板宽度 */
    UPROPERTY()
    USizeBox* RootSizeBox;

    /** 面板背景边框 */
    UPROPERTY()
    UBorder* PanelBackground;

    /** 面板容器 */
    UPROPERTY()
    UVerticalBox* PanelContainer;

    //--- 任务图预览区 ---

    /** 任务图预览区边框 */
    UPROPERTY()
    UBorder* TaskGraphSectionBorder;

    /** 任务图预览标题 */
    UPROPERTY()
    UTextBlock* TaskGraphTitle;

    /** 任务图预览组件 */
    UPROPERTY()
    UMATaskGraphPreview* TaskGraphPreview;

    //--- 技能列表预览区 ---

    /** 技能列表预览区边框 */
    UPROPERTY()
    UBorder* SkillListSectionBorder;

    /** 技能列表预览标题 */
    UPROPERTY()
    UTextBlock* SkillListTitle;

    /** 技能列表预览组件 */
    UPROPERTY()
    UMASkillListPreview* SkillListPreview;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 创建任务图预览区 */
    UWidget* CreateTaskGraphSection();

    /** 创建技能列表预览区 */
    UWidget* CreateSkillListSection();

    /** 创建分隔线 */
    UWidget* CreateSeparator();
};
