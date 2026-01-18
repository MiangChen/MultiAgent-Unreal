// MASkillListModal.h
// 技能列表模态窗口 - 用于查看和编辑技能分配
// 继承自 MABaseModalWidget，提供技能列表详细可视化
// Requirements: 5.2, 5.3, 6.1, 6.4

#pragma once

#include "CoreMinimal.h"
#include "MABaseModalWidget.h"
#include "../Core/Types/MATaskGraphTypes.h"
#include "../Core/Comm/MACommTypes.h"
#include "MASkillListModal.generated.h"

class UMAGanttCanvas;
class UMASkillAllocationModel;
class UMultiLineEditableTextBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class UScrollBox;

//=============================================================================
// 委托声明
//=============================================================================

/** 技能列表确认提交委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillListConfirmed, const FMASkillAllocationData&, Data);

/** 技能列表拒绝委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillListRejected);

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMASkillListModal, Log, All);

//=============================================================================
// UMASkillListModal - 技能列表模态窗口
//=============================================================================

/**
 * 技能列表模态窗口
 * 
 * 功能:
 * - 显示技能分配的详细甘特图可视化
 * - 支持只读模式（查看）和编辑模式
 * - 在编辑模式下可以修改技能分配
 * - 提供 Confirm/Reject/Edit 按钮操作
 * 
 * 布局结构:
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                        Skill List                                    │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │  ┌─────────────────────────────────────────────────────────────┐   │
 * │  │                    Gantt Canvas                              │   │
 * │  │  Time Step 0  1  2  3  4  ...                                │   │
 * │  │  ├──────────────────────────────────────────────────────────┤   │
 * │  │  │ UAV_01   [■][■][■]                                       │   │
 * │  │  │ UAV_02   [■][■][■]                                       │   │
 * │  │  │ UGV_01      [■■■■]                                       │   │
 * │  │  │ Humanoid [■][■■■]                                        │   │
 * │  │  └──────────────────────────────────────────────────────────┘   │
 * │  └─────────────────────────────────────────────────────────────┘   │
 * │  ┌─────────────────────────────────────────────────────────────┐   │
 * │  │ JSON Preview (编辑模式下可编辑)                              │   │
 * │  └─────────────────────────────────────────────────────────────┘   │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │                    [Confirm] [Reject] [Edit]                        │
 * └─────────────────────────────────────────────────────────────────────┘
 * 
 * Requirements: 5.2, 5.3, 6.1, 6.4
 */
UCLASS()
class MULTIAGENT_API UMASkillListModal : public UMABaseModalWidget
{
    GENERATED_BODY()

public:
    UMASkillListModal(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 加载技能分配数据
     * @param Data 技能分配数据
     */
    UFUNCTION(BlueprintCallable, Category = "SkillListModal")
    void LoadSkillAllocation(const FMASkillAllocationData& Data);

    /**
     * 从 JSON 字符串加载技能分配
     * @param JsonString JSON 字符串
     * @return 是否加载成功
     */
    UFUNCTION(BlueprintCallable, Category = "SkillListModal")
    bool LoadSkillAllocationFromJson(const FString& JsonString);

    /**
     * 获取当前技能分配数据
     * @return 当前技能分配数据
     */
    UFUNCTION(BlueprintPure, Category = "SkillListModal")
    FMASkillAllocationData GetSkillAllocationData() const;

    /**
     * 获取数据模型
     * @return 技能分配数据模型
     */
    UFUNCTION(BlueprintPure, Category = "SkillListModal")
    UMASkillAllocationModel* GetAllocationModel() const { return AllocationModel; }

    /**
     * 获取甘特图画布
     * @return 甘特图画布 Widget
     */
    UFUNCTION(BlueprintPure, Category = "SkillListModal")
    UMAGanttCanvas* GetGanttCanvas() const { return GanttCanvas; }

    /**
     * 获取技能分配消息
     * @return 技能分配消息结构
     */
    UFUNCTION(BlueprintPure, Category = "SkillListModal")
    FMASkillAllocationMessage GetSkillAllocationMessage() const;

    /**
     * 更新技能执行状态
     * 由 MAUIManager 在技能状态变化时调用
     * @param TimeStep 时间步
     * @param RobotId 机器人 ID
     * @param NewStatus 新的执行状态
     */
    UFUNCTION(BlueprintCallable, Category = "SkillListModal")
    void UpdateSkillStatus(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus);

    //=========================================================================
    // 委托
    //=========================================================================

    /** 技能列表确认提交委托 */
    UPROPERTY(BlueprintAssignable, Category = "SkillListModal|Events")
    FOnSkillListConfirmed OnSkillListConfirmed;

    /** 技能列表拒绝委托 */
    UPROPERTY(BlueprintAssignable, Category = "SkillListModal|Events")
    FOnSkillListRejected OnSkillListRejected;

protected:
    //=========================================================================
    // UMABaseModalWidget 重写
    //=========================================================================

    /** 编辑模式变化回调 */
    virtual void OnEditModeChanged(bool bEditable) override;

    /** 获取模态标题 */
    virtual FText GetModalTitleText() const override;

    /** 构建内容区域 */
    virtual void BuildContentArea(UVerticalBox* InContentContainer) override;

    /** 主题应用后回调 */
    virtual void OnThemeApplied() override;

    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 创建甘特图画布区域 */
    UBorder* CreateGanttCanvasArea();

    /** 创建 JSON 预览区域 */
    UVerticalBox* CreateJsonPreviewArea();

    /** 同步 JSON 预览内容 */
    void SyncJsonPreview();

    /** 从 JSON 编辑器更新模型 */
    void UpdateModelFromJson();

    /** 处理确认按钮点击 */
    UFUNCTION()
    void HandleConfirm();

    /** 处理拒绝按钮点击 */
    UFUNCTION()
    void HandleReject();

    //=========================================================================
    // 事件处理
    //=========================================================================

    /** 数据模型变更回调 */
    UFUNCTION()
    void OnModelDataChanged();

    /** "更新" 按钮点击回调 (编辑模式) */
    UFUNCTION()
    void OnUpdateButtonClicked();

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 甘特图画布 Widget */
    UPROPERTY()
    UMAGanttCanvas* GanttCanvas;

    /** 甘特图画布容器 */
    UPROPERTY()
    UBorder* GanttCanvasContainer;

    /** JSON 预览/编辑文本框 */
    UPROPERTY()
    UMultiLineEditableTextBox* JsonPreviewBox;

    /** JSON 预览容器 */
    UPROPERTY()
    UVerticalBox* JsonPreviewContainer;

    /** JSON 预览标签 */
    UPROPERTY()
    UTextBlock* JsonPreviewLabel;

    /** "更新" 按钮 (编辑模式) */
    UPROPERTY()
    UButton* UpdateButton;

    /** JSON 预览滚动容器 */
    UPROPERTY()
    UScrollBox* JsonScrollBox;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 技能分配数据模型 */
    UPROPERTY()
    UMASkillAllocationModel* AllocationModel;

    /** 原始数据 (用于拒绝时恢复) */
    FMASkillAllocationData OriginalData;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 甘特图画布高度比例 */
    float GanttCanvasHeightRatio = 0.7f;

    /** JSON 预览高度比例 */
    float JsonPreviewHeightRatio = 0.3f;
};
