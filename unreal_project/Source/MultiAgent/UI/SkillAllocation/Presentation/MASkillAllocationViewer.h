// MASkillAllocationViewer.h
// 技能分配查看器主容器 Widget - 纯 C++ 实现

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"
#include "../../../Core/Comm/Domain/MACommSkillTypes.h"
#include "MASkillAllocationViewer.generated.h"

struct FMASkillAllocationViewerActionResult;
class UMAGanttCanvas;
class UMASkillAllocationModel;
class FMASkillAllocationViewerRuntimeAdapter;
class UMultiLineEditableTextBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class USizeBox;
class UScrollBox;
class UMAStyledButton;

//=============================================================================
// 委托声明
//=============================================================================

/** 技能分配变更委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillAllocationChanged, const FMASkillAllocationData&, NewData);

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
 * 布局结构 (左右分栏):
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                  Skill Allocation Workbench                  [✕]   │
 * ├──────────────────────────────┬──────────────────────────────────────┤
 * │   Left: JSON Editor          │    Right: Gantt Canvas               │
 * │  ┌────────────────────────┐  │  ┌───────────────────────────────┐ │
 * │  │ {                      │  │  │ Time Step 0  1  2  3 ...      │ │
 * │  │   "timeSteps": [...],  │  │  ├───────────────────────────────┤ │
 * │  │   "robots":    [...]   │  │  │ UAV-1   [■][■][■]             │ │
 * │  │ }                      │  │  │ UGV-1      [■■■■]             │ │
 * │  │         (editable)     │  │  │ Humanoid [■][■■■]             │ │
 * │  └────────────────────────┘  │  └───────────────────────────────┘ │
 * │  [ Update ] [ Save ]          │                                     │
 * └──────────────────────────────┴──────────────────────────────────────┘
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

    /** 追加诊断日志消息 (输出到控制台，带时间戳) */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    void AppendStatusLog(const FString& Message);

    /** 获取 JSON 编辑器文本 */
    UFUNCTION(BlueprintPure, Category = "SkillAllocation")
    FString GetJsonText() const;

    /** 设置 JSON 编辑器文本 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    void SetJsonText(const FString& JsonText);

    /** 获取数据模型 */
    UFUNCTION(BlueprintPure, Category = "SkillAllocation")
    UMASkillAllocationModel* GetAllocationModel() const { return AllocationModel; }

    /** 获取甘特图画布 */
    UFUNCTION(BlueprintPure, Category = "SkillAllocation")
    UMAGanttCanvas* GetGanttCanvas() const { return GanttCanvas; }

    /** 加载 Mock 数据 (从 datasets/skill_allocation_example.json) */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    bool LoadMockData();

    /** 应用主题样式 */
    UFUNCTION(BlueprintCallable, Category = "SkillAllocation")
    void ApplyTheme(class UMAUITheme* InTheme);

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

    /** 创建左侧面板 (JSON 编辑器 + 按钮) */
    UBorder* CreateLeftPanel();

    /** 创建右侧面板 (甘特图画布) */
    UBorder* CreateRightPanel();

    /** 创建 JSON 编辑器区域 */
    UVerticalBox* CreateJsonEditorSection();

    //=========================================================================
    // 事件处理
    //=========================================================================

    /** "更新技能列表" 按钮点击回调 */
    UFUNCTION()
    void OnUpdateButtonClicked();

    /** "保存" 按钮点击回调 */
    UFUNCTION()
    void OnSaveButtonClicked();

    /** 保存数据并导航到 Modal */
    void SaveAndNavigateToModal();

    /** 应用 coordinator 返回的结果 */
    void ApplyActionResult(const FMASkillAllocationViewerActionResult& Result);

    /** "重置" 按钮点击回调 */
    UFUNCTION()
    void OnResetButtonClicked();

    /** 关闭按钮点击回调 */
    UFUNCTION()
    void OnCloseButtonClicked();

    /** 数据模型变更回调 */
    UFUNCTION()
    void OnModelDataChanged();

    /** 技能状态更新回调 */
    UFUNCTION()
    void OnSkillStatusUpdated(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus Status);

    /** 运行时技能分配变更回调 */
    UFUNCTION()
    void OnRuntimeSkillAllocationChanged(const FMASkillAllocationData& NewData);

    /** 运行时技能状态更新回调 */
    UFUNCTION()
    void OnRuntimeSkillStatusUpdated(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus);

    //=========================================================================
    // 拖拽事件处理
    //=========================================================================

    /** 甘特图拖拽开始回调 - 当拖拽操作开始时调用 */
    UFUNCTION()
    void OnGanttDragStarted(const FString& SkillName, int32 TimeStep, const FString& RobotId);

    /** 甘特图拖拽完成回调 - 当技能块成功移动到新位置时调用 */
    UFUNCTION()
    void OnGanttDragCompleted(int32 SourceTimeStep, const FString& SourceRobotId,
                              int32 TargetTimeStep, const FString& TargetRobotId);

    /** 甘特图拖拽取消回调 */
    UFUNCTION()
    void OnGanttDragCancelled();

    /** 甘特图拖拽被阻止回调 - 当执行期间尝试拖拽时调用 */
    UFUNCTION()
    void OnGanttDragBlocked();

    /** 甘特图拖拽失败回调 - 当拖拽操作因无效目标失败时调用 */
    UFUNCTION()
    void OnGanttDragFailed();

    /** 从运行时存储加载技能分配 */
    void LoadRuntimeSkillAllocation();

    /** 绑定运行时事件 */
    void BindRuntimeEvents();

    /** 同步数据到运行时存储 */
    bool PersistRuntimeData(FString* OutError = nullptr);

    //=========================================================================
    // 辅助方法
    //=========================================================================

    /** 同步 JSON 编辑器内容 (从模型) */
    void SyncJsonEditorFromModel();

    /** 获取当前时间戳字符串 */
    FString GetTimestamp() const;

    //=========================================================================
    // UI 组件
    //=========================================================================

    /** JSON 编辑器文本框 (可编辑) */
    UPROPERTY()
    UMultiLineEditableTextBox* JsonEditorBox;

    /** "更新技能列表" 按钮 */
    UPROPERTY()
    UMAStyledButton* UpdateButton;

    /** "保存" 按钮 (黄色，保存并返回 Modal) */
    UPROPERTY()
    UMAStyledButton* SaveButton;

    /** "重置" 按钮 */
    UPROPERTY()
    UButton* ResetButton;

    /** 关闭按钮 (右上角 X) */
    UPROPERTY()
    UButton* CloseButton;

    /** 甘特图画布 Widget */
    UPROPERTY()
    UMAGanttCanvas* GanttCanvas;

    //=========================================================================
    // 主题相关 TextBlock 引用 (用于 ApplyTheme 更新颜色)
    //=========================================================================

    /** 标题文字 */
    UPROPERTY()
    UTextBlock* TitleText;

    /** 提示文字 */
    UPROPERTY()
    UTextBlock* HintText;

    /** 关闭按钮文字 */
    UPROPERTY()
    UTextBlock* CloseText;

    /** JSON 编辑器标签 */
    UPROPERTY()
    UTextBlock* JsonEditorLabel;

    /** 背景遮罩 */
    UPROPERTY()
    UBorder* BackgroundOverlay;

    /** 缓存的主题引用 */
    UPROPERTY()
    class UMAUITheme* Theme;

    //=========================================================================
    // 数据
    //=========================================================================

    /** 技能分配数据模型 */
    UPROPERTY()
    UMASkillAllocationModel* AllocationModel;

    friend class FMASkillAllocationViewerRuntimeAdapter;

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
    FLinearColor LabelColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);

    /** 按钮颜色 */
    FLinearColor ButtonColor = FLinearColor(0.2f, 0.5f, 0.8f, 1.0f);
};
