// MAUIManager.h
// UI Manager - 负责 Widget 的生命周期管理和创建
// 从 MAHUD 分离出 Widget 管理职责，降低代码复杂度
// 集成 HUD 状态管理器，处理 UI 状态转换和模态窗口管理

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MAHUDTypes.h"
#include "../../Core/Types/MATaskGraphTypes.h"
#include "MAUIManager.generated.h"

// 前向声明
class UMATaskPlannerWidget;
class UMASkillAllocationViewer;
class UMASimpleMainWidget;
class UMADirectControlIndicator;
class UMAEmergencyWidget;
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
class UMAEmergencyModal;
class UMABaseModalWidget;
class UMANotificationWidget;

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
    SimpleMain      UMETA(DisplayName = "Simple Main (Legacy)"),
    SemanticMap     UMETA(DisplayName = "Semantic Map"),
    DirectControl   UMETA(DisplayName = "Direct Control"),
    Emergency       UMETA(DisplayName = "Emergency"),
    Modify          UMETA(DisplayName = "Modify"),
    Edit            UMETA(DisplayName = "Edit"),
    SceneList       UMETA(DisplayName = "Scene List")
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

    /** 获取 SimpleMainWidget 实例 (Legacy) */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMASimpleMainWidget* GetSimpleMainWidget() const;

    /** 获取 DirectControlIndicator 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMADirectControlIndicator* GetDirectControlIndicator() const;

    /** 获取 EmergencyWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMAEmergencyWidget* GetEmergencyWidget() const;

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
    // HUD 状态管理 (Requirements: 2.1)
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

    /** 获取紧急事件模态窗口 */
    UFUNCTION(BlueprintPure, Category = "UI|Modal")
    UMAEmergencyModal* GetEmergencyModal() const { return EmergencyModal; }

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

    //=========================================================================
    // 通知处理 (Requirements: 4.1, 4.2, 4.3, 4.5)
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
     * 获取通知类型对应的模态类型
     * @param NotificationType 通知类型
     * @return 对应的模态类型
     */
    UFUNCTION(BlueprintPure, Category = "UI|Notification")
    EMAModalType GetModalTypeForNotification(EMANotificationType NotificationType) const;

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
    UMASimpleMainWidget* SimpleMainWidget;

    UPROPERTY()
    UMADirectControlIndicator* DirectControlIndicator;

    UPROPERTY()
    UMAEmergencyWidget* EmergencyWidget;

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

    /** 当前 UI 主题 */
    UPROPERTY()
    UMAUITheme* CurrentTheme;

    /** 默认 UI 主题 (当未指定主题时使用) */
    UPROPERTY()
    UMAUITheme* DefaultTheme;

    //=========================================================================
    // HUD 状态管理 (Requirements: 2.1)
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

    /** 紧急事件模态窗口 */
    UPROPERTY()
    UMAEmergencyModal* EmergencyModal;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 获取 Widget 的 Z-Order
     * @param Type Widget 类型
     * @return Z-Order 值
     */
    int32 GetWidgetZOrder(EMAWidgetType Type) const;

    /**
     * 设置输入模式为 UI 和游戏混合
     * @param Widget 要聚焦的 Widget
     */
    void SetInputModeGameAndUI(UUserWidget* Widget);

    /**
     * 恢复输入模式为纯游戏
     */
    void SetInputModeGameOnly();

    //=========================================================================
    // HUD 状态变化处理 (Requirements: 2.3, 2.4, 2.5)
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
     * HUD 状态变化回调
     * @param OldState 变化前的状态
     * @param NewState 变化后的状态
     */
    UFUNCTION()
    void OnHUDStateChanged(EMAHUDState OldState, EMAHUDState NewState);

    /**
     * 通知接收回调
     * @param Type 通知类型
     */
    UFUNCTION()
    void OnNotificationReceived(EMANotificationType Type);

    /**
     * 模态确认回调
     * @param ModalType 模态类型
     */
    UFUNCTION()
    void OnModalConfirmed(EMAModalType ModalType);

    /**
     * 模态拒绝回调
     * @param ModalType 模态类型
     */
    UFUNCTION()
    void OnModalRejected(EMAModalType ModalType);

    /**
     * 模态编辑请求回调
     * @param ModalType 模态类型
     */
    UFUNCTION()
    void OnModalEditRequested(EMAModalType ModalType);

    /**
     * 模态隐藏动画完成回调
     * 当用户点击 ❌ 关闭按钮后，动画完成时调用
     * 用于同步 HUDStateManager 状态
     */
    UFUNCTION()
    void OnModalHideAnimationComplete();

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

    /**
     * 任务图数据变化回调
     * @param Data 新的任务图数据
     */
    UFUNCTION()
    void OnTempTaskGraphChanged(const FMATaskGraphData& Data);

    /**
     * 技能分配数据变化回调
     * @param Data 新的技能分配数据
     */
    UFUNCTION()
    void OnTempSkillAllocationChanged(const FMASkillAllocationData& Data);

    /**
     * 技能状态实时更新回调
     * @param TimeStep 时间步
     * @param RobotId 机器人 ID
     * @param NewStatus 新状态
     */
    UFUNCTION()
    void OnSkillStatusUpdated(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus);
};
