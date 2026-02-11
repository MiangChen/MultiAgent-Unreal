// MAHUD.h
// HUD 管理器 - 管理 HUD 绘制和事件协调
// 继承自 MASelectionHUD 以保留框选绘制功能
// Widget 管理职责已委托给 UMAUIManager

#pragma once

#include "CoreMinimal.h"
#include "MASelectionHUD.h"
#include "../Core/MAHUDTypes.h"
#include "MAHUD.generated.h"

// 前向声明 - Widget 类 (用于事件回调参数类型)
class UMASimpleMainWidget;
class UMATaskPlannerWidget;
class UMASkillAllocationViewer;
class UMADirectControlIndicator;
class UMAModifyWidget;
class UMAEditWidget;
class UMASceneListWidget;

// 前向声明 - 其他类
class UMAUIManager;
class AMAPlayerController;
class AMACharacter;
class UMAEditModeManager;
class UMACameraSensorComponent;
class UMASceneGraphManager;
class AMAPointOfInterest;
class UMAUITheme;
class UMAHUDStateManager;
class UMAMainHUDWidget;
struct FMAPlannerResponse;
struct FMATaskGraphData;
struct FMASceneGraphNode;
struct FMATaskPlan;
struct FMASkillListMessage;
struct FMASkillAllocationData;

/**
 * HUD 管理器
 * 
 * 职责:
 * - HUD 绘制 (DrawHUD, DrawNotification, DrawSceneLabels, DrawEditModeIndicator)
 * - 事件绑定和协调
 * - 委托 Widget 创建和管理给 UIManager
 * 
 */
UCLASS()
class MULTIAGENT_API AMAHUD : public AMASelectionHUD
{
    GENERATED_BODY()

public:
    AMAHUD();

    //=========================================================================
    // UI Manager
    //=========================================================================

    /** UI Manager 实例 - 负责 Widget 生命周期管理 */
    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UMAUIManager* UIManager;

    /**
     * 获取 UI Manager
     * @return UI Manager 指针，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMAUIManager* GetUIManager() const { return UIManager; }

    //=========================================================================
    // Widget 类引用 (在蓝图中设置) - 委托给 UIManager
    //=========================================================================

    /** SemanticMap Widget 类引用 (后续阶段) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Classes")
    TSubclassOf<UUserWidget> SemanticMapWidgetClass;

    /** UI 主题数据资产 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Classes")
    UMAUITheme* UIThemeAsset;

    //=========================================================================
    // HUD 状态管理访问
    //=========================================================================

    /**
     * 获取 HUD 状态管理器
     * @return HUD 状态管理器指针，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI|HUD")
    UMAHUDStateManager* GetHUDStateManager() const;

    /**
     * 获取主 HUD Widget
     * @return 主 HUD Widget 指针，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI|HUD")
    UMAMainHUDWidget* GetMainHUDWidget() const;

    //=========================================================================
    // UI 状态
    //=========================================================================

    /** MainWidget 是否可见 */
    UPROPERTY(BlueprintReadOnly, Category = "UI State")
    bool bMainUIVisible = false;

    /** 是否正在显示场景标签 */
    UPROPERTY(BlueprintReadOnly, Category = "UI State|SceneLabel")
    bool bShowingSceneLabels = false;

    /** 缓存的场景节点数据 */
    UPROPERTY(BlueprintReadOnly, Category = "UI State|SceneLabel")
    TArray<FMASceneGraphNode> CachedSceneNodes;

    //=========================================================================
    // 公共接口 (委托到 UIManager)
    //=========================================================================

    /**
     * 切换 MainWidget 的显示/隐藏
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ToggleMainUI();

    /**
     * 显示 MainWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ShowMainUI();

    /**
     * 隐藏 MainWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void HideMainUI();

    /**
     * 切换语义地图 Widget 的显示/隐藏 (后续阶段)
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ToggleSemanticMap();

    /**
     * 显示 Direct Control 指示器
     * @param Agent 当前控制的 Agent
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ShowDirectControlIndicator(AMACharacter* Agent);

    /**
     * 隐藏 Direct Control 指示器
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void HideDirectControlIndicator();

    /**
     * 检查 Direct Control 指示器是否可见
     * @return true 如果指示器当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsDirectControlIndicatorVisible() const;

    /**
     * 获取 SimpleMainWidget 实例 (已弃用，通过 UIManager 访问)
     * @return SimpleMainWidget 指针，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMASimpleMainWidget* GetSimpleMainWidget() const;

    /**
     * 获取 TaskPlannerWidget 实例 (通过 UIManager 访问)
     * @return TaskPlannerWidget 指针，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMATaskPlannerWidget* GetTaskPlannerWidget() const;

    /**
     * 切换技能分配查看器的显示/隐藏
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ToggleSkillAllocationViewer();

    /**
     * 显示技能分配查看器
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ShowSkillAllocationViewer();

    /**
     * 隐藏技能分配查看器
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void HideSkillAllocationViewer();

    /**
     * 检查技能分配查看器是否可见
     * @return true 如果查看器当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsSkillAllocationViewerVisible() const;

    /**
     * 加载任务图数据到 TaskPlannerWidget
     * @param Data 任务图数据
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void LoadTaskGraph(const FMATaskGraphData& Data);

    /**
     * 检查 MainUI 是否可见
     * @return true 如果 MainWidget 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsMainUIVisible() const { return bMainUIVisible; }

    //=========================================================================
    // 突发事件 UI 控制
    //=========================================================================
    // Modify 模式 UI 控制
    //=========================================================================

    /**
     * 显示 ModifyWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Modify")
    void ShowModifyWidget();

    /**
     * 隐藏 ModifyWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Modify")
    void HideModifyWidget();

    /**
     * 检查 ModifyWidget 是否可见
     * @return true 如果 ModifyWidget 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Modify")
    bool IsModifyWidgetVisible() const;

    //=========================================================================
    // Edit 模式 UI 控制
    //=========================================================================

    /**
     * 显示 EditWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Edit")
    void ShowEditWidget();

    /**
     * 隐藏 EditWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Edit")
    void HideEditWidget();

    /**
     * 检查 EditWidget 是否可见
     * @return true 如果 EditWidget 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Edit")
    bool IsEditWidgetVisible() const;

    /**
     * 检查鼠标是否在 EditWidget 上
     * 用于 Edit 模式下判断是否应该处理场景点击
     * @return true 如果鼠标在 EditWidget 或 SceneListWidget 上
     */
    UFUNCTION(BlueprintPure, Category = "UI|Edit")
    bool IsMouseOverEditWidget() const;

    //=========================================================================
    // System Log Panel 控制
    //=========================================================================

    /**
     * 显示 SystemLogPanel
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Panel")
    void ShowSystemLogPanel();

    /**
     * 隐藏 SystemLogPanel
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Panel")
    void HideSystemLogPanel();

    /**
     * 切换 SystemLogPanel 的显示/隐藏
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Panel")
    void ToggleSystemLogPanel();

    /**
     * 检查 SystemLogPanel 是否可见
     * @return true 如果 SystemLogPanel 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Panel")
    bool IsSystemLogPanelVisible() const;

    //=========================================================================
    // Preview Panel 控制
    //=========================================================================

    /**
     * 显示 PreviewPanel
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Panel")
    void ShowPreviewPanel();

    /**
     * 隐藏 PreviewPanel
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Panel")
    void HidePreviewPanel();

    /**
     * 切换 PreviewPanel 的显示/隐藏
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Panel")
    void TogglePreviewPanel();

    /**
     * 检查 PreviewPanel 是否可见
     * @return true 如果 PreviewPanel 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Panel")
    bool IsPreviewPanelVisible() const;

    //=========================================================================
    // Instruction Panel 控制
    //=========================================================================

    /**
     * 显示 InstructionPanel
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Panel")
    void ShowInstructionPanel();

    /**
     * 隐藏 InstructionPanel
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Panel")
    void HideInstructionPanel();

    /**
     * 切换 InstructionPanel 的显示/隐藏
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Panel")
    void ToggleInstructionPanel();

    /**
     * 检查 InstructionPanel 是否可见
     * @return true 如果 InstructionPanel 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Panel")
    bool IsInstructionPanelVisible() const;

    /**
     * 检查鼠标是否在右侧边栏上
     * 用于判断右键是否应该用于视角旋转
     * @return true 如果鼠标在右侧边栏区域上
     */
    UFUNCTION(BlueprintPure, Category = "UI|HUD")
    bool IsMouseOverRightSidebar() const;

    /**
     * 检查鼠标是否在任何常驻 UI 上
     * 包括右侧边栏、小地图等
     * @return true 如果鼠标在任何常驻 UI 上
     */
    UFUNCTION(BlueprintPure, Category = "UI|HUD")
    bool IsMouseOverPersistentUI() const;

    //=========================================================================
    // 场景标签可视化
    //=========================================================================

    /**
     * 开始场景标签可视化
     * 加载场景图数据并开始显示标签
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|SceneLabel")
    void StartSceneLabelVisualization();

    /**
     * 停止场景标签可视化
     * 停止显示标签并清空缓存
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|SceneLabel")
    void StopSceneLabelVisualization();

    /**
     * 检查场景标签可视化是否激活
     * @return true 如果正在显示场景标签
     */
    UFUNCTION(BlueprintPure, Category = "UI|SceneLabel")
    bool IsSceneLabelVisualizationActive() const { return bShowingSceneLabels; }

    //=========================================================================
    // 通知系统
    //=========================================================================

    /**
     * 显示通知消息
     * @param Message 通知消息内容
     * @param bIsError 是否为错误消息 (红色)，false 为成功消息 (绿色)
     * @param bIsWarning 是否为警告消息 (黄色)，优先级低于 bIsError
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Notification")
    void ShowNotification(const FString& Message, bool bIsError = false, bool bIsWarning = false);

protected:
    //=========================================================================
    // AHUD 重写
    //=========================================================================

    virtual void BeginPlay() override;
    virtual void DrawHUD() override;

    //=========================================================================
    // 事件回调
    //=========================================================================

    /**
     * PlayerController 的 MainUI 切换事件回调
     * @param bVisible 是否显示
     */
    UFUNCTION()
    void OnMainUIToggled(bool bVisible);

    /**
     * PlayerController 的重新聚焦事件回调
     */
    UFUNCTION()
    void OnRefocusMainUI();

private:
    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 绑定 PlayerController 事件
     */
    void BindControllerEvents();

    /**
     * 绑定 Widget 委托 (在 UIManager 创建 Widget 后调用)
     */
    void BindWidgetDelegates();

    /**
     * 获取拥有此 HUD 的 MAPlayerController
     * @return MAPlayerController 指针，可能为 nullptr
     */
    AMAPlayerController* GetMAPlayerController() const;

    /**
     * SimpleMainWidget 指令提交回调
     * @param Command 用户输入的指令
     */
    UFUNCTION()
    void OnSimpleCommandSubmitted(const FString& Command);

    /**
     * CommSubsystem 响应回调
     * @param Response 规划器响应
     */
    UFUNCTION()
    void OnPlannerResponse(const FMAPlannerResponse& Response);

    /**
     * ModifyWidget 确认修改回调 (单选模式)
     * @param Actor 选中的 Actor
     * @param LabelText 文本框内容
     */
    UFUNCTION()
    void OnModifyConfirmed(AActor* Actor, const FString& LabelText);

    /**
     * ModifyWidget 确认修改回调 (多选模式)
     * @param Actors 选中的 Actor 数组
     * @param LabelText 文本框内容
     * @param GeneratedJson 生成的 JSON 字符串
     */
    UFUNCTION()
    void OnMultiSelectModifyConfirmed(const TArray<AActor*>& Actors, const FString& LabelText, const FString& GeneratedJson);

    /**
     * PlayerController 的 Actor 选中回调 (单选模式)
     * @param SelectedActor 选中的 Actor
     */
    UFUNCTION()
    void OnModifyActorSelected(AActor* SelectedActor);

    /**
     * PlayerController 的 Actor 选中回调 (多选模式)
     * @param SelectedActors 选中的 Actor 数组
     */
    UFUNCTION()
    void OnModifyActorsSelected(const TArray<AActor*>& SelectedActors);

    //=========================================================================
    // 通知系统内部 - 已迁移到 HUDWidget
    // 以下成员变量不再使用，保留注释说明
    //=========================================================================

    //=========================================================================
    // 场景标签可视化内部方法
    //=========================================================================

    /**
     * 加载场景图数据用于可视化
     * 从 SceneGraphManager 获取所有节点并缓存
     */
    void LoadSceneGraphForVisualization();

    /**
     * 绘制场景标签
     * 在 DrawHUD() 中调用，遍历缓存的节点并绘制绿色文本
     */
    void DrawSceneLabels();

    /**
     * 绘制画中画相机
     * 在 DrawHUD() 中调用，使用 Canvas 绘制所有活动的画中画
     */
    void DrawPIPCameras();

    /**
     * 清除高亮的 Actor
     * 通过 PlayerController 清除当前高亮
     */
    void ClearHighlightedActor();

    //=========================================================================
    // Edit 模式内部方法
    //=========================================================================

    /**
     * 绑定 EditModeManager 事件
     */
    void BindEditModeManagerEvents();

    /**
     * 绑定后端事件
     * 绑定 CommSubsystem 事件到 HUD 状态管理器
     */
    void BindBackendEvents();

    /**
     * 绑定模态窗口委托
     * 绑定模态确认/拒绝委托到后端提交
     */
    void BindModalDelegates();

    /**
     * 绑定 EditWidget 委托
     */
    void BindEditWidgetDelegates();

    /**
     * 收集 Edit 模式数据并更新 HUDWidget
     * 不再使用 Canvas 绘制，改为更新 HUDWidget 的 POI/Goal/Zone 列表
     */
    void DrawEditModeIndicator();

    /**
     * EditModeManager 选择变化回调
     */
    UFUNCTION()
    void OnEditModeSelectionChanged();

    /**
     * EditModeManager 临时场景图变化回调
     */
    UFUNCTION()
    void OnTempSceneGraphChanged();

    /**
     * EditWidget 确认修改回调
     * @param Actor 选中的 Actor
     * @param JsonContent JSON 编辑内容
     */
    UFUNCTION()
    void OnEditConfirmed(AActor* Actor, const FString& JsonContent);

    /**
     * EditWidget 删除 Actor 回调
     * @param Actor 要删除的 Actor
     */
    UFUNCTION()
    void OnEditDeleteActor(AActor* Actor);

    /**
     * EditWidget 创建 Goal 回调
     * @param POI 用于创建 Goal 的 POI
     * @param Description Goal 描述
     */
    UFUNCTION()
    void OnEditCreateGoal(AMAPointOfInterest* POI, const FString& Description);

    /**
     * EditWidget 创建 Zone 回调
     * @param POIs 用于创建 Zone 的 POI 数组
     * @param Description Zone 描述
     */
    UFUNCTION()
    void OnEditCreateZone(const TArray<AMAPointOfInterest*>& POIs, const FString& Description);

    /**
     * EditWidget 添加预设 Actor 回调
     * @param POI 目标 POI
     * @param ActorType 预设 Actor 类型
     */
    UFUNCTION()
    void OnEditAddPresetActor(AMAPointOfInterest* POI, const FString& ActorType);

    /**
     * EditWidget 删除 POI 回调
     * @param POIs 要删除的 POI 数组
     */
    UFUNCTION()
    void OnEditDeletePOIs(const TArray<AMAPointOfInterest*>& POIs);

    /**
     * EditWidget 设为 Goal 回调
     * @param Actor 目标 Actor
     */
    UFUNCTION()
    void OnEditSetAsGoal(AActor* Actor);

    /**
     * EditWidget 取消 Goal 回调
     * @param Actor 目标 Actor
     */
    UFUNCTION()
    void OnEditUnsetAsGoal(AActor* Actor);

    /**
     * SceneListWidget Goal 项点击回调
     * @param GoalId Goal Node ID
     */
    UFUNCTION()
    void OnSceneListGoalClicked(const FString& GoalId);

    /**
     * SceneListWidget Zone 项点击回调
     * @param ZoneId Zone Node ID
     */
    UFUNCTION()
    void OnSceneListZoneClicked(const FString& ZoneId);

    //=========================================================================
    // 后端事件回调
    //=========================================================================

    /**
     * 任务图更新回调
     * @param TaskPlan 任务规划 DAG 数据
     */
    UFUNCTION()
    void OnTaskGraphReceived(const FMATaskPlan& TaskPlan);

    /**
     * 技能分配数据更新回调 (用于 UI 交互流程)
     * @param AllocationData 技能分配数据
     */
    UFUNCTION()
    void OnSkillAllocationReceived(const FMASkillAllocationData& AllocationData);
    /**
     * 技能列表更新回调
     * @param SkillList 技能列表消息
     * @param bExecutable 是否为可执行的最终技能列表
     */
    UFUNCTION()
    void OnSkillListReceived(const FMASkillListMessage& SkillList, bool bExecutable);

    /**
     * 模态确认回调
     * @param ModalType 模态类型
     */
    UFUNCTION()
    void OnModalConfirmedHandler(EMAModalType ModalType);

    /**
     * 模态拒绝回调
     * @param ModalType 模态类型
     */
    UFUNCTION()
    void OnModalRejectedHandler(EMAModalType ModalType);
};
