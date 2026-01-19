// MAHUDStateManager.h
// HUD 状态管理器 - 负责协调所有 UI 状态、输入处理和状态转换
// 实现状态机驱动的 UI 流程控制

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MAHUDTypes.h"
#include "MAHUDStateManager.generated.h"

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMAHUDState, Log, All);

//=============================================================================
// 委托声明
//=============================================================================

/**
 * HUD 状态变化委托
 * @param OldState 变化前的状态
 * @param NewState 变化后的状态
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHUDStateChanged, EMAHUDState, OldState, EMAHUDState, NewState);

/**
 * 通知接收委托
 * @param Type 通知类型
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNotificationReceived, EMANotificationType, Type);

/**
 * 模态操作委托
 * @param ModalType 模态类型
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModalConfirmed, EMAModalType, ModalType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModalRejected, EMAModalType, ModalType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModalEditRequested, EMAModalType, ModalType);

//=============================================================================
// UMAHUDStateManager - HUD 状态管理器
//=============================================================================

/**
 * HUD 状态管理器
 * 
 * 职责:
 * - 维护 HUD 状态 (NormalHUD, NotificationPending, ReviewModal, EditingModal)
 * - 处理输入动作 (Z/N/X 键)
 * - 触发状态转换动画
 * - 确保同一时间只有一个模态窗口可见
 * - 在模态关闭时恢复之前的状态
 * 
 * 使用方式:
 * - 由 MAUIManager 或 MAHUD 持有
 * - 通过 TransitionToState() 进行状态转换
 * - 绑定 OnStateChanged 委托响应状态变化
 * - 通过 Handle*Input() 方法处理用户输入
 * 
 * Requirements: 2.1, 2.3, 2.5
 */
UCLASS(BlueprintType)
class MULTIAGENT_API UMAHUDStateManager : public UObject
{
    GENERATED_BODY()

public:
    UMAHUDStateManager();

    //=========================================================================
    // 委托
    //=========================================================================

    /** 状态变化委托 - 当 HUD 状态改变时触发 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnHUDStateChanged OnStateChanged;

    /** 通知接收委托 - 当收到新通知时触发 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNotificationReceived OnNotificationReceived;

    /** 模态确认委托 - 当用户点击确认按钮时触发 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModalConfirmed OnModalConfirmed;

    /** 模态拒绝委托 - 当用户点击拒绝按钮时触发 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModalRejected OnModalRejected;

    /** 模态编辑请求委托 - 当用户点击编辑按钮时触发 */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnModalEditRequested OnModalEditRequested;

    //=========================================================================
    // 状态查询
    //=========================================================================

    /**
     * 获取当前 HUD 状态
     * @return 当前状态
     */
    UFUNCTION(BlueprintPure, Category = "State")
    EMAHUDState GetCurrentState() const { return CurrentState; }

    /**
     * 获取当前活动的模态类型
     * @return 模态类型，如果没有活动模态则返回 None
     */
    UFUNCTION(BlueprintPure, Category = "State")
    EMAModalType GetActiveModalType() const { return ActiveModalType; }

    /**
     * 获取待处理的通知类型
     * @return 通知类型，如果没有待处理通知则返回 None
     */
    UFUNCTION(BlueprintPure, Category = "State")
    EMANotificationType GetPendingNotification() const { return PendingNotification; }

    /**
     * 检查是否有模态窗口处于活动状态
     * @return true 如果有模态窗口可见
     */
    UFUNCTION(BlueprintPure, Category = "State")
    bool IsModalActive() const;

    /**
     * 检查当前是否处于编辑模式
     * @return true 如果当前状态为 EditingModal
     */
    UFUNCTION(BlueprintPure, Category = "State")
    bool IsInEditMode() const { return CurrentState == EMAHUDState::EditingModal; }

    //=========================================================================
    // 状态转换
    //=========================================================================

    /**
     * 转换到指定状态
     * @param NewState 目标状态
     * @param ModalType 模态类型 (仅在转换到模态状态时需要)
     * @return true 如果转换成功
     */
    UFUNCTION(BlueprintCallable, Category = "State")
    bool TransitionToState(EMAHUDState NewState, EMAModalType ModalType = EMAModalType::None);

    /**
     * 返回到正常 HUD 状态
     * 关闭所有模态窗口和通知
     */
    UFUNCTION(BlueprintCallable, Category = "State")
    void ReturnToNormalHUD();

    //=========================================================================
    // 通知处理
    //=========================================================================

    /**
     * 显示通知
     * @param Type 通知类型
     */
    UFUNCTION(BlueprintCallable, Category = "Notification")
    void ShowNotification(EMANotificationType Type);

    /**
     * 关闭当前通知
     */
    UFUNCTION(BlueprintCallable, Category = "Notification")
    void DismissNotification();

    /**
     * 检查是否有待处理的通知
     * @return true 如果有待处理通知
     */
    UFUNCTION(BlueprintPure, Category = "Notification")
    bool HasPendingNotification() const { return PendingNotification != EMANotificationType::None; }

    //=========================================================================
    // 输入处理 (Requirements: 2.2, 10.1, 10.2, 10.3, 10.4)
    //=========================================================================

    /**
     * 处理检查任务图输入 (Z 键)
     * - 如果有任务图通知，打开任务图模态
     * - 如果任务图模态已打开，关闭它
     */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void HandleCheckTaskInput();

    /**
     * 处理检查技能列表输入 (N 键)
     * - 如果有技能列表通知，打开技能列表模态
     * - 如果技能列表模态已打开，关闭它
     */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void HandleCheckSkillInput();

    /**
     * 处理检查突发事件输入 (X 键)
     * - 如果有突发事件，直接打开编辑模态
     * - 如果突发事件模态已打开，关闭它
     */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void HandleCheckEmergencyInput();

    //=========================================================================
    // 模态操作
    //=========================================================================

    /**
     * 模态确认操作
     * 关闭模态并触发确认委托
     */
    UFUNCTION(BlueprintCallable, Category = "Modal")
    void OnModalConfirm();

    /**
     * 模态拒绝操作
     * 关闭模态并触发拒绝委托
     */
    UFUNCTION(BlueprintCallable, Category = "Modal")
    void OnModalReject();

    /**
     * 模态编辑操作
     * 从查看模态转换到编辑模态
     */
    UFUNCTION(BlueprintCallable, Category = "Modal")
    void OnModalEdit();

    //=========================================================================
    // 状态转换验证
    //=========================================================================

    /**
     * 检查状态转换是否有效
     * @param FromState 起始状态
     * @param ToState 目标状态
     * @return true 如果转换有效
     */
    UFUNCTION(BlueprintPure, Category = "State")
    bool IsValidTransition(EMAHUDState FromState, EMAHUDState ToState) const;

private:
    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 当前 HUD 状态 */
    UPROPERTY()
    EMAHUDState CurrentState;

    /** 当前活动的模态类型 */
    UPROPERTY()
    EMAModalType ActiveModalType;

    /** 待处理的通知类型 */
    UPROPERTY()
    EMANotificationType PendingNotification;

    /** 延迟通知队列 - 当 modal 打开时收到的通知会被加入队列 */
    UPROPERTY()
    TArray<EMANotificationType> DeferredNotifications;

    /** 状态转换历史 (用于调试) */
    UPROPERTY()
    TArray<FMAHUDStateTransition> TransitionHistory;

    /** 最大历史记录数 */
    static constexpr int32 MaxHistorySize = 20;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 设置状态 (内部使用)
     * @param NewState 新状态
     */
    void SetState(EMAHUDState NewState);

    /**
     * 记录状态转换
     * @param FromState 起始状态
     * @param ToState 目标状态
     * @param ModalType 模态类型
     */
    void RecordTransition(EMAHUDState FromState, EMAHUDState ToState, EMAModalType ModalType);

    /**
     * 获取通知对应的模态类型
     * @param NotificationType 通知类型
     * @return 对应的模态类型
     */
    EMAModalType GetModalTypeForNotification(EMANotificationType NotificationType) const;

    /**
     * 处理延迟的通知队列
     * 在 modal 关闭后调用，显示队列中的第一个通知
     */
    void ProcessDeferredNotifications();
};
