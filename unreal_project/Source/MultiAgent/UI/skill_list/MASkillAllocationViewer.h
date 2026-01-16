// MASkillAllocationViewer.h
// 技能分配查看器主容器 Widget - 纯 C++ 实现

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "../../Core/Comm/MACommTypes.h"
#include "../../Core/Manager/MACommandManager.h"
#include "MASkillAllocationViewer.generated.h"

class UMAGanttCanvas;
class UMASkillAllocationModel;
class UMATempDataManager;
class UMultiLineEditableTextBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UScrollBox;

//=============================================================================
// 委托声明
//=============================================================================

/** 技能分配变更委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillAllocationChanged, const FMASkillAllocationData&, NewData);

/** 执行开始委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExecutionStarted);

/** 执行完成委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExecutionCompleted);

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMASkillAllocationViewer, Log, All);

//=============================================================================
// UMASkillAllocationViewer - 技能分配查看器主容器
//=============================================================================

/**
 * 技能分配查看器主容器 Widget
 * 
 * 布局结构:
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                  Skill Allocation Viewer                            │
 * ├─────────────────────┬───────────────────────────────────────────────┤
 * │   Left Panel        │         Right Panel                           │
 * │  ┌───────────────┐  │  ┌─────────────────────────────────────────┐ │
 * │  │ Status Log    │  │  │    Gantt Canvas                         │ │
 * │  │ (ReadOnly)    │  │  │  Time Step 0  1  2  3  4  ...           │ │
 * │  └───────────────┘  │  │  ├──────────────────────────────────────┤ │
 * │  ┌───────────────┐  │  │  │ UAV_01   [■][■][■]                  │ │
 * │  │ JSON Editor   │  │  │  │ UAV_02   [■][■][■]                  │ │
 * │  │ (Editable)    │  │  │  │ UGV_01      [■■■■]                  │ │
 * │  └───────────────┘  │  │  │ Humanoid [■][■■■]                  │ │
 * │  ┌───────────────┐  │  │  └──────────────────────────────────────┘ │
 * │  │ Update Button │  │  │                                             │
 * │  └───────────────┘  │  │  Legend:                                    │
 * │  ┌───────────────┐  │  │  ■ Gray (Pending)                           │
 * │  │ Start Execute │  │  │  ■ Yellow (In-Progress)                     │
 * │  └───────────────┘  │  │  ■ Green (Completed)                        │
 * └─────────────────────┴──────────────────────────────────────────────┘
 * 
 */
UCLASS()
class MULTIAGENT_API UMASkillAllocationViewer : public UUserWidget
{
    GENERATED_BODY()

public:
    UMASkillAllocationViewer(const FObjectInitializer& ObjectInitializer);

    //=========================================================================
    // 公共接口
    //=========================================================================

    /** 加载技能分配数据 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    void LoadSkillAllocation(const FMASkillAllocationData& Data);

    /** 从 JSON 字符串加载技能分配 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    bool LoadSkillAllocationFromJson(const FString& JsonString);

    /** 追加状态日志消息 (带时间戳) */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    void AppendStatusLog(const FString& Message);

    /** 清空状态日志 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    void ClearStatusLog();

    /** 获取 JSON 编辑器文本 */
    UFUNCTION(BlueprintPure, Category = "SkillAllocation")
    FString GetJsonText() const;

    /** 设置 JSON 编辑器文本 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    void SetJsonText(const FString& JsonText);

    /** 开始执行 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    void StartExecution();

    /** 检查是否正在执行 */
    UFUNCTION(BlueprintPure, Category = "SkillAllocation")
    bool IsExecuting() const { return bIsExecuting; }

    /** 获取数据模型 */
    UFUNCTION(BlueprintPure, Category = "SkillAllocation")
    UMASkillAllocationModel* GetAllocationModel() const { return AllocationModel; }

    /** 获取甘特图画布 */
    UFUNCTION(BlueprintPure, Category = "SkillAllocation")
    UMAGanttCanvas* GetGanttCanvas() const { return GanttCanvas; }

    /** 加载 Mock 数据 (从 datasets/skill_allocation_example.json) */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    bool LoadMockData();

    /** 将 FMASkillAllocationData 转换为 FMASkillListMessage 格式
     * @param InData 输入的技能分配数据
     * @param OutMessage 输出的技能列表消息
     * @param OutErrorMessage 错误信息（如果转换失败）
     * @return 转换是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    static bool ConvertToSkillListMessage(
        const FMASkillAllocationData& InData,
        FMASkillListMessage& OutMessage,
        FString& OutErrorMessage);

    //=========================================================================
    // 委托
    //=========================================================================

    /** 技能分配变更委托 */
    UPROPERTY(BlueprintAssignable, Category = "SkillAllocation|Events")
    FOnSkillAllocationChanged OnSkillAllocationChanged;

    /** 执行开始委托 */
    UPROPERTY(BlueprintAssignable, Category = "SkillAllocation|Events")
    FOnExecutionStarted OnExecutionStarted;

    /** 执行完成委托 */
    UPROPERTY(BlueprintAssignable, Category = "SkillAllocation|Events")
    FOnExecutionCompleted OnExecutionCompletedDelegate;

protected:
    //=========================================================================
    // UUserWidget 重写
    //=========================================================================

    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    //=========================================================================
    // UI 构建
    //=========================================================================

    /** 构建 UI 布局 */
    void BuildUI();

    /** 创建左侧面板 */
    UBorder* CreateLeftPanel();

    /** 创建右侧面板 */
    UBorder* CreateRightPanel();

    /** 创建状态日志区域 */
    UVerticalBox* CreateStatusLogSection();

    /** 创建 JSON 编辑器区域 */
    UVerticalBox* CreateJsonEditorSection();

    //=========================================================================
    // 事件处理
    //=========================================================================

    /** "更新技能列表" 按钮点击回调 */
    UFUNCTION()
    void OnUpdateButtonClicked();

    /** "开始执行" 按钮点击回调 */
    UFUNCTION()
    void OnStartExecuteButtonClicked();

    /** "重置" 按钮点击回调 */
    UFUNCTION()
    void OnResetButtonClicked();

    /** 数据模型变更回调 */
    UFUNCTION()
    void OnModelDataChanged();

    /** 技能状态更新回调 */
    UFUNCTION()
    void OnSkillStatusUpdated(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus Status);

    /** TempDataManager 技能列表变更回调 */
    UFUNCTION()
    void OnTempSkillListChanged(const FMASkillAllocationData& NewData);

    //=========================================================================
    // 辅助方法
    //=========================================================================

    /** 同步 JSON 编辑器内容 (从模型) */
    void SyncJsonEditorFromModel();

    /** 获取当前时间戳字符串 */
    FString GetTimestamp() const;

    /** 检查是否所有技能都已完成 */
    bool AreAllSkillsCompleted() const;

    /** 执行完成回调 */
    void OnExecutionCompleted();

    /** 执行失败回调 */
    void OnExecutionFailed(int32 TimeStep, const FString& RobotId);

    /** 重置执行状态 */
    void ResetExecution();

    //=========================================================================
    // MACommandManager 事件绑定
    //=========================================================================

    /** 绑定 MACommandManager 事件 */
    void BindCommandManagerEvents();

    /** 解绑 MACommandManager 事件 */
    void UnbindCommandManagerEvents();

    /** MACommandManager 时间步完成回调 */
    UFUNCTION()
    void OnCommandManagerTimeStepCompleted(const FMATimeStepFeedback& Feedback);

    /** MACommandManager 技能列表完成回调 */
    UFUNCTION()
    void OnCommandManagerSkillListCompleted(const TArray<FMATimeStepFeedback>& AllFeedbacks);

    //=========================================================================
    // 模拟执行 (已废弃 - 现在使用真实执行)
    //=========================================================================

    // 注意: StartSimulatedExecution(), OnSimulationTick(), AdvanceSimulation() 已移除
    // 现在通过 MACommandManager::ExecuteSkillList() 进行真实执行

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** 状态日志文本框 (只读) */
    UPROPERTY()
    UMultiLineEditableTextBox* StatusLogBox;

    /** 状态日志滚动容器 */
    UPROPERTY()
    UScrollBox* StatusLogScrollBox;

    /** JSON 编辑器文本框 (可编辑) */
    UPROPERTY()
    UMultiLineEditableTextBox* JsonEditorBox;

    /** "更新技能列表" 按钮 */
    UPROPERTY()
    UButton* UpdateButton;

    /** "开始执行" 按钮 */
    UPROPERTY()
    UButton* StartExecuteButton;

    /** "重置" 按钮 */
    UPROPERTY()
    UButton* ResetButton;

    /** 甘特图画布 Widget */
    UPROPERTY()
    UMAGanttCanvas* GanttCanvas;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 技能分配数据模型 */
    UPROPERTY()
    UMASkillAllocationModel* AllocationModel;

    /** 是否正在执行 */
    bool bIsExecuting = false;

    //=========================================================================
    // 模拟执行状态 (已废弃 - 保留变量以避免编译错误，但不再使用)
    //=========================================================================

    /** 模拟执行定时器句柄 (已废弃) */
    FTimerHandle SimulationTimerHandle;

    /** 当前模拟的时间步 (已废弃) */
    int32 CurrentSimulationTimeStep = 0;

    /** 当前时间步的状态 (已废弃) */
    int32 CurrentSimulationPhase = 0;

    /** 模拟执行间隔 (已废弃) */
    float SimulationTickInterval = 1.0f;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 左侧面板宽度比例 */
    float LeftPanelWidthRatio = 0.35f;

    /** 状态日志高度比例 */
    float StatusLogHeightRatio = 0.3f;

    //=========================================================================
    // 颜色配置
    //=========================================================================

    /** 背景颜色 */
    FLinearColor BackgroundColor = FLinearColor(0.05f, 0.05f, 0.08f, 0.98f);

    /** 面板背景颜色 */
    FLinearColor PanelBackgroundColor = FLinearColor(0.08f, 0.08f, 0.1f, 1.0f);

    /** 标题颜色 */
    FLinearColor TitleColor = FLinearColor(0.9f, 0.9f, 1.0f, 1.0f);

    /** 标签颜色 */
    FLinearColor LabelColor = FLinearColor(0.7f, 0.7f, 0.8f, 1.0f);

    /** 按钮颜色 */
    FLinearColor ButtonColor = FLinearColor(0.2f, 0.5f, 0.8f, 1.0f);
};
