// MAUIManager.h
// UI Manager - 负责 Widget 的生命周期管理和创建
// 从 MAHUD 分离出 Widget 管理职责，降低代码复杂度
// 集成 HUD 状态管理器，处理 UI 状态转换和模态窗口管理

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MAHUDTypes.h"
#include "Core/TaskGraph/Domain/MATaskGraphTypes.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"
#include "Application/MAUIStateModalCoordinator.h"
#include "Application/MAUIRuntimeEventCoordinator.h"
#include "Application/MAUIWidgetLifecycleCoordinator.h"
#include "Application/MAUIWidgetInteractionCoordinator.h"
#include "Application/MAUIThemeCoordinator.h"
#include "Application/MAUINotificationCoordinator.h"
#include "Application/MAUIWidgetRegistryCoordinator.h"
#include "Infrastructure/MAUIInputModeAdapter.h"
#include "Infrastructure/MAUIRuntimeBridge.h"
#include "MAUIManager.generated.h"

// 前向声明
class UMATaskPlannerWidget;
class UMASkillAllocationViewer;
class UMADirectControlIndicator;
class UMAModifyWidget;
class UMAEditWidget;
class UMASceneListWidget;
class UMAHUDWidget;
class UMAUITheme;
class UUserWidget;
class UMAHUDStateManager;
class UMAMainHUDWidget;
class UMATaskGraphModal;
class UMASkillAllocationModal;
class UMABaseModalWidget;
class UMADecisionModal;
class UMASystemLogPanel;
class UMAPreviewPanel;
class UMAInstructionPanel;

//=============================================================================
// Widget 类型枚举
//=============================================================================

/**
 * Widget 类型枚举
 * 用于标识不同类型的 Widget，便于统一管理
 */
UENUM(BlueprintType)
enum class EMAWidgetType : uint8
{
    None            UMETA(DisplayName = "None"),
    HUD             UMETA(DisplayName = "HUD Overlay"),
    TaskPlanner     UMETA(DisplayName = "Task Planner"),
    SkillAllocation UMETA(DisplayName = "Skill Allocation"),
    SimpleMain      UMETA(Hidden),
    SemanticMap     UMETA(DisplayName = "Semantic Map"),
    DirectControl   UMETA(DisplayName = "Direct Control"),
    Modify          UMETA(DisplayName = "Modify"),
    Edit            UMETA(DisplayName = "Edit"),
    SceneList       UMETA(DisplayName = "Scene List"),
    SystemLogPanel   UMETA(DisplayName = "System Log Panel"),
    PreviewPanel     UMETA(DisplayName = "Preview Panel"),
    InstructionPanel UMETA(DisplayName = "Instruction Panel")
};

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMAUIManager, Log, All);

//=============================================================================
// UMAUIManager - Widget 管理器
//=============================================================================

/**
 * UI Manager - Widget 管理器
 * 
 * 职责:
 * - 创建所有 Widget 实例
 * - 提供 Widget 访问接口
 * - 管理 Widget 可见性状态
 * - 处理输入模式切换
 * 
 * 使用方式:
 * - 由 MAHUD 持有，生命周期与 MAHUD 一致
 * - 在 MAHUD::BeginPlay 中调用 Initialize() 和 CreateAllWidgets()
 * - 通过 GetWidget() 或类型安全 Getter 访问 Widget
 * - 通过 ShowWidget() / HideWidget() / ToggleWidget() 控制可见性
 */
UCLASS()
class MULTIAGENT_API UMAUIManager : public UObject
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 初始化
    //=========================================================================

    /**
     * 初始化 UI Manager
     * @param InOwningPC 拥有此 Manager 的 PlayerController
     */
    void Initialize(APlayerController* InOwningPC);

    /**
     * 创建所有 Widget 实例
     * 必须在 Initialize() 之后调用
     */
    void CreateAllWidgets();

    //=========================================================================
    // Coordinator 内部接口（受控写入/装配）
    //=========================================================================

    /** 获取 OwningPC（用于 Coordinator 装配） */
    APlayerController* GetOwningPlayerController() const { return OwningPC; }

    /** 获取 SemanticMap Widget 类（用于生命周期创建） */
    TSubclassOf<UUserWidget> GetSemanticMapWidgetClass() const { return SemanticMapWidgetClass; }

    /** 设置 HUDStateManager（统一写入口） */
    void SetHUDStateManagerInternal(UMAHUDStateManager* InHUDStateManager);

    /** 设置 MainHUDWidget（统一写入口） */
    void SetMainHUDWidgetInternal(UMAMainHUDWidget* InMainHUDWidget);

    /** 按 WidgetType 设置类型化 Widget 指针（统一写入口） */
    void SetWidgetInstanceInternal(EMAWidgetType Type, UUserWidget* Widget);

    /** 按 ModalType 设置类型化 Modal 指针（统一写入口） */
    void SetModalWidgetInternal(EMAModalType ModalType, UMABaseModalWidget* ModalWidget);

    /** 注册 WidgetType 到 Widgets 映射（统一写入口） */
    void RegisterWidgetMappingInternal(EMAWidgetType Type, UUserWidget* Widget);

    /** 已注册 Widget 数量（用于日志） */
    int32 GetRegisteredWidgetCountInternal() const { return Widgets.Num(); }

    /** 统一绑定 runtime 事件源（TempData/Comm/Command） */
    void BindRuntimeEventSourcesInternal();

    /** 运行时事件桥接：TempData */
    bool BindTempDataEventsInternal(FString& OutError);

    /** 运行时事件桥接：Comm */
    bool BindCommEventsInternal(FString& OutError);

    /** 运行时事件桥接：Command */
    bool BindCommandEventsInternal(FString& OutError);

    /** 运行时数据访问：TaskGraph */
    bool LoadTaskGraphDataInternal(FMATaskGraphData& OutData, FString& OutError) const;

    /** 运行时数据访问：SkillAllocation */
    bool LoadSkillAllocationDataInternal(FMASkillAllocationData& OutData, FString& OutError) const;

    /** 运行时动作：发送决策回复 */
    bool SendDecisionResponseInternal(const FString& SelectedOption, const FString& UserFeedback, FString& OutError) const;

    /** 运行时动作：清理恢复通知计时器 */
    bool ClearResumeNotificationTimerInternal(FString& OutError);

    /** 运行时动作：调度恢复通知自动隐藏 */
    bool ScheduleResumeNotificationAutoHideInternal(float DelaySeconds, FString& OutError);

    /** 运行时配置：是否显示阶段提示气泡（读 simulation.show_notification，缺失时回退 true） */
    bool IsPhaseNotificationEnabledInternal() const;

    /** 统一创建模态窗口（供生命周期协调器调用） */
    void CreateModalWidgetsInternal();

    /** 统一注册并添加到 Viewport（通过 WidgetRegistryCoordinator） */
    void RegisterViewportWidgetInternal(EMAWidgetType Type, UUserWidget* Widget, ESlateVisibility Visibility);

    /** 统一注册但不添加到 Viewport（通过 WidgetRegistryCoordinator） */
    void RegisterManagedWidgetInternal(EMAWidgetType Type, UUserWidget* Widget, ESlateVisibility Visibility);

    /** Theme 写入口：CurrentTheme */
    void SetCurrentThemeInternal(UMAUITheme* InTheme);

    /** Theme 写入口：DefaultTheme */
    void SetDefaultThemeInternal(UMAUITheme* InTheme);

    /** Theme 读入口：DefaultTheme */
    UMAUITheme* GetDefaultThemeInternal() const { return DefaultTheme; }

    /** Resume 通知定时器访问入口 */
    FTimerHandle& GetResumeNotificationTimerHandleInternal() { return ResumeNotificationTimerHandle; }

    //=========================================================================
    // Widget 访问 - 通用接口
    //=========================================================================

    /**
     * 获取指定类型的 Widget
     * @param Type Widget 类型
     * @return Widget 指针，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    UUserWidget* GetWidget(EMAWidgetType Type) const;

    //=========================================================================
    // Widget 访问 - 类型安全 Getter
    //=========================================================================

    /** 获取 TaskPlannerWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMATaskPlannerWidget* GetTaskPlannerWidget() const;

    /** 获取 SkillAllocationViewer 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMASkillAllocationViewer* GetSkillAllocationViewer() const;

    /** 获取 DirectControlIndicator 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMADirectControlIndicator* GetDirectControlIndicator() const;


    /** 获取 ModifyWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMAModifyWidget* GetModifyWidget() const;

    /** 获取 EditWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMAEditWidget* GetEditWidget() const;

    /** 获取 SceneListWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMASceneListWidget* GetSceneListWidget() const;

    /** 获取 HUDWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMAHUDWidget* GetHUDWidget() const;

    //=========================================================================
    // Widget 访问 - 右侧边栏拆分面板
    //=========================================================================

    /** 获取 SystemLogPanel 实例 */
    UFUNCTION(BlueprintPure, Category = "UI|Panel")
    UMASystemLogPanel* GetSystemLogPanel() const;

    /** 获取 PreviewPanel 实例 */
    UFUNCTION(BlueprintPure, Category = "UI|Panel")
    UMAPreviewPanel* GetPreviewPanel() const;

    /** 获取 InstructionPanel 实例 */
    UFUNCTION(BlueprintPure, Category = "UI|Panel")
    UMAInstructionPanel* GetInstructionPanel() const;

    //=========================================================================
    // HUD 状态管理
    //=========================================================================

    /** 获取 HUD 状态管理器 */
    UFUNCTION(BlueprintPure, Category = "UI|HUD")
    UMAHUDStateManager* GetHUDStateManager() const { return HUDStateManager; }

    /** 获取主 HUD Widget */
    UFUNCTION(BlueprintPure, Category = "UI|HUD")
    UMAMainHUDWidget* GetMainHUDWidget() const { return MainHUDWidget; }

    //=========================================================================
    // 模态窗口访问
    //=========================================================================

    /** 获取任务图模态窗口 */
    UFUNCTION(BlueprintPure, Category = "UI|Modal")
    UMATaskGraphModal* GetTaskGraphModal() const { return TaskGraphModal; }

    /** 获取技能列表模态窗口 */
    UFUNCTION(BlueprintPure, Category = "UI|Modal")
    UMASkillAllocationModal* GetSkillAllocationModal() const { return SkillAllocationModal; }


    /** 获取决策模态窗口 */
    UFUNCTION(BlueprintPure, Category = "UI|Modal")
    UMADecisionModal* GetDecisionModal() const { return DecisionModal; }

    /** 根据类型获取模态窗口 */
    UFUNCTION(BlueprintPure, Category = "UI|Modal")
    UMABaseModalWidget* GetModalByType(EMAModalType ModalType) const;

    //=========================================================================
    // Widget 可见性控制
    //=========================================================================

    /**
     * 显示指定类型的 Widget
     * @param Type Widget 类型
     * @param bSetFocus 是否设置焦点 (默认 true)
     * @return 是否成功显示
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    bool ShowWidget(EMAWidgetType Type, bool bSetFocus = true);

    /**
     * 隐藏指定类型的 Widget
     * @param Type Widget 类型
     * @return 是否成功隐藏
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    bool HideWidget(EMAWidgetType Type);

    /**
     * 切换指定类型 Widget 的可见性
     * @param Type Widget 类型
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleWidget(EMAWidgetType Type);

    /**
     * 检查指定类型的 Widget 是否可见
     * @param Type Widget 类型
     * @return 是否可见
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsWidgetVisible(EMAWidgetType Type) const;

    /**
     * 检查是否有全屏 Widget 或 Modal 可见
     * 用于决定是否应该绘制 3D 调试信息（如选中圆环、头顶状态文字）
     * @return 是否有全屏 Widget 可见
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsAnyFullscreenWidgetVisible() const;

    /**
     * 从 SkillAllocationViewer 导航到 SkillAllocationModal
     * 隐藏 Viewer 并显示 Modal，从 TempDataManager 加载数据
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Navigation")
    void NavigateFromViewerToSkillAllocationModal();

    /**
     * 从 TaskPlannerWidget 导航到 TaskGraphModal
     * 隐藏 Workbench 并显示 Modal，从 TempDataManager 加载数据
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Navigation")
    void NavigateFromWorkbenchToTaskGraphModal();

    /**
     * 设置 Widget 焦点
     * @param Type Widget 类型
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetWidgetFocus(EMAWidgetType Type);

    //=========================================================================
    // SemanticMap Widget 类引用 (蓝图设置)
    //=========================================================================

    /** SemanticMap Widget 类引用 (后续阶段) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Classes")
    TSubclassOf<UUserWidget> SemanticMapWidgetClass;

    //=========================================================================
    // UI 主题
    //=========================================================================

    /**
     * 加载 UI 主题
     * @param ThemeAsset 主题数据资产，nullptr 时使用默认主题
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Theme")
    void LoadTheme(UMAUITheme* ThemeAsset);

    /**
     * 获取当前 UI 主题
     * @return 当前主题，如果未加载则返回默认主题
     */
    UFUNCTION(BlueprintPure, Category = "UI|Theme")
    UMAUITheme* GetTheme() const;

    /**
     * 验证主题完整性
     * @param InTheme 要验证的主题
     * @return true 如果主题有效
     */
    UFUNCTION(BlueprintPure, Category = "UI|Theme")
    bool ValidateTheme(UMAUITheme* InTheme) const;

    /**
     * 分发主题到所有已注册的 Widget
     * 遍历所有 Widget 并调用其 ApplyTheme 方法
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Theme")
    void DistributeThemeToWidgets();

    //=========================================================================
    // 主题变更委托
    //=========================================================================

    /** 主题变更委托 - 当主题加载或变更时广播 */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThemeChanged, UMAUITheme*, NewTheme);

    /** 主题变更事件 - Widget 可订阅此事件以接收主题更新 */
    UPROPERTY(BlueprintAssignable, Category = "UI|Theme")
    FOnThemeChanged OnThemeChanged;

    //=========================================================================
    // 通知处理
    //=========================================================================

    /**
     * 显示通知
     * @param Type 通知类型
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Notification")
    void ShowNotification(EMANotificationType Type);

    /**
     * 关闭当前通知
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Notification")
    void DismissNotification();

    /**
     * 关闭索要用户指令通知
     * 仅当当前显示的是 RequestUserCommand 类型通知时才关闭
     * 用于用户发送指令后自动关闭通知
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Notification")
    void DismissRequestUserCommandNotification();

    /**
     * 获取通知类型对应的模态类型
     * @param NotificationType 通知类型
     * @return 对应的模态类型
     */
    UFUNCTION(BlueprintPure, Category = "UI|Notification")
    EMAModalType GetModalTypeForNotification(EMANotificationType NotificationType) const;

    //=========================================================================
    // Coordinator / Delegate 回调桥接（显式公开接口）
    //=========================================================================

    /** 设置输入模式为 UI 和游戏混合 */
    void SetInputModeGameAndUI(UUserWidget* Widget);

    /** 恢复输入模式为纯游戏 */
    void SetInputModeGameOnly();

    /** HUD 状态变化回调 */
    UFUNCTION()
    void OnHUDStateChanged(EMAHUDState OldState, EMAHUDState NewState);

    /** 通知接收回调 */
    UFUNCTION()
    void OnNotificationReceived(EMANotificationType Type);

    /** 模态确认回调 */
    UFUNCTION()
    void OnModalConfirmed(EMAModalType ModalType);

    /** 模态拒绝回调 */
    UFUNCTION()
    void OnModalRejected(EMAModalType ModalType);

    /** 模态编辑请求回调 */
    UFUNCTION()
    void OnModalEditRequested(EMAModalType ModalType);

    /** 模态隐藏动画完成回调 */
    UFUNCTION()
    void OnModalHideAnimationComplete();

    /** TempDataManager: 任务图数据变化回调 */
    UFUNCTION()
    void OnTempTaskGraphChanged(const FMATaskGraphData& Data);

    /** TempDataManager: 技能分配数据变化回调 */
    UFUNCTION()
    void OnTempSkillAllocationChanged(const FMASkillAllocationData& Data);

    /** TempDataManager: 技能状态实时更新回调 */
    UFUNCTION()
    void OnSkillStatusUpdated(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus);

    /** CommandManager: 执行暂停状态变化回调 */
    UFUNCTION()
    void OnExecutionPauseStateChanged(bool bPaused);

    /** CommSubsystem: 收到索要用户指令请求回调 */
    UFUNCTION()
    void OnRequestUserCommandReceived();

    /** CommSubsystem: 收到决策请求回调 */
    UFUNCTION()
    void OnDecisionDataReceived(const FString& Description, const FString& ContextJson);

    /** 决策模态确认回调 */
    UFUNCTION()
    void OnDecisionModalConfirmed(const FString& SelectedOption, const FString& UserFeedback);

    /** 决策模态拒绝回调 */
    UFUNCTION()
    void OnDecisionModalRejected(const FString& SelectedOption, const FString& UserFeedback);

private:
    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 拥有此 Manager 的 PlayerController */
    UPROPERTY()
    APlayerController* OwningPC;

    /** Widget 实例映射 */
    UPROPERTY()
    TMap<EMAWidgetType, UUserWidget*> Widgets;

    //=========================================================================
    // Widget 实例 (类型安全存储)
    //=========================================================================

    UPROPERTY()
    UMATaskPlannerWidget* TaskPlannerWidget;

    UPROPERTY()
    UMASkillAllocationViewer* SkillAllocationViewer;

    UPROPERTY()
    UMADirectControlIndicator* DirectControlIndicator;

    UPROPERTY()
    UMAModifyWidget* ModifyWidget;

    UPROPERTY()
    UMAEditWidget* EditWidget;

    UPROPERTY()
    UMASceneListWidget* SceneListWidget;

    UPROPERTY()
    UMAHUDWidget* HUDWidget;

    UPROPERTY()
    UUserWidget* SemanticMapWidget;

    //=========================================================================
    // 右侧边栏拆分面板实例
    //=========================================================================

    /** 系统日志面板 */
    UPROPERTY()
    UMASystemLogPanel* SystemLogPanel;

    /** 预览面板 */
    UPROPERTY()
    UMAPreviewPanel* PreviewPanel;

    /** 指令输入面板 */
    UPROPERTY()
    UMAInstructionPanel* InstructionPanel;

    /** 当前 UI 主题 */
    UPROPERTY()
    UMAUITheme* CurrentTheme;

    /** 默认 UI 主题 (当未指定主题时使用) */
    UPROPERTY()
    UMAUITheme* DefaultTheme;

    //=========================================================================
    // HUD 状态管理
    //=========================================================================

    /** HUD 状态管理器 */
    UPROPERTY()
    UMAHUDStateManager* HUDStateManager;

    /** 主 HUD Widget */
    UPROPERTY()
    UMAMainHUDWidget* MainHUDWidget;

    //=========================================================================
    // 模态窗口实例
    //=========================================================================

    /** 任务图模态窗口 */
    UPROPERTY()
    UMATaskGraphModal* TaskGraphModal;

    /** 技能列表模态窗口 */
    UPROPERTY()
    UMASkillAllocationModal* SkillAllocationModal;


    /** 决策模态窗口 */
    UPROPERTY()
    UMADecisionModal* DecisionModal;

    //=========================================================================
    // 内部方法
    //=========================================================================

    //=========================================================================
    // HUD 状态变化处理
    //=========================================================================

    /**
     * 初始化 HUD 状态管理器
     */
    void InitializeHUDStateManager();

    /**
     * 创建模态窗口实例
     */
    void CreateModalWidgets();

    /**
     * 绑定状态管理器委托
     */
    void BindStateManagerDelegates();

    /**
     * 显示指定类型的模态窗口
     * @param ModalType 模态类型
     * @param bEditMode 是否为编辑模式
     */
    void ShowModal(EMAModalType ModalType, bool bEditMode);

    /**
     * 隐藏当前模态窗口
     */
    void HideCurrentModal();

    /**
     * 加载数据到模态窗口
     * @param ModalType 模态类型
     */
    void LoadDataIntoModal(EMAModalType ModalType);

    /**
     * 获取模态窗口的 Z-Order
     * @return Z-Order 值
     */
    int32 GetModalZOrder() const;

    //=========================================================================
    // TempDataManager 事件绑定
    //=========================================================================

    /**
     * 绑定 TempDataManager 数据变化事件
     */
    void BindTempDataManagerEvents();

    //=========================================================================
    // CommSubsystem 事件绑定
    //=========================================================================

    /**
     * 绑定 CommSubsystem 委托事件
     */
    void BindCommSubsystemEvents();

    /**
     * 绑定 CommandManager 委托事件
     * 监听 OnExecutionPauseStateChanged 委托，暂停/恢复时显示通知
     */
    void BindCommandManagerEvents();

    /** 恢复通知自动隐藏定时器 */
    FTimerHandle ResumeNotificationTimerHandle;

    /** L1 Application: HUD 状态机 + Modal 流程协调器 */
    FMAUIStateModalCoordinator StateModalCoordinator;

    /** L1 Application: TempData/Comm/Command 事件桥接协调器 */
    FMAUIRuntimeEventCoordinator RuntimeEventCoordinator;

    /** L1 Application: Widget 生命周期创建协调器 */
    FMAUIWidgetLifecycleCoordinator WidgetLifecycleCoordinator;

    /** L1 Application: Widget 注册与层级策略协调器 */
    FMAUIWidgetRegistryCoordinator WidgetRegistryCoordinator;

    /** L1 Application: Widget 交互与导航协调器 */
    FMAUIWidgetInteractionCoordinator WidgetInteractionCoordinator;

    /** L1 Application: Theme 协调器 */
    FMAUIThemeCoordinator ThemeCoordinator;

    /** L1 Application: 通知协调器 */
    FMAUINotificationCoordinator NotificationCoordinator;

    /** L3 Infrastructure: 输入模式适配器 */
    FMAUIInputModeAdapter InputModeAdapter;

    /** L4 Infrastructure: UI 运行时桥接 */
    FMAUIRuntimeBridge RuntimeBridge;
};
