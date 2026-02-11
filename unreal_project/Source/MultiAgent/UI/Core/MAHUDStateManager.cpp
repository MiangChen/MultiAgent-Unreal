// MAHUDStateManager.cpp
// HUD 状态管理器实现

#include "MAHUDStateManager.h"

//=============================================================================
// 日志类别定义
//=============================================================================
DEFINE_LOG_CATEGORY(LogMAHUDState);

//=============================================================================
// 构造函数
//=============================================================================

UMAHUDStateManager::UMAHUDStateManager()
    : CurrentState(EMAHUDState::NormalHUD)
    , ActiveModalType(EMAModalType::None)
    , PendingNotification(EMANotificationType::None)
{
}

//=============================================================================
// 状态查询
//=============================================================================

bool UMAHUDStateManager::IsModalActive() const
{
    return CurrentState == EMAHUDState::ReviewModal || CurrentState == EMAHUDState::EditingModal;
}

//=============================================================================
// 状态转换
//=============================================================================

bool UMAHUDStateManager::TransitionToState(EMAHUDState NewState, EMAModalType ModalType)
{
    // 验证状态转换有效性
    if (!IsValidTransition(CurrentState, NewState))
    {
        UE_LOG(LogMAHUDState, Warning, TEXT("Invalid state transition from %s to %s"),
            *UEnum::GetValueAsString(CurrentState),
            *UEnum::GetValueAsString(NewState));
        return false;
    }

    // 验证模态类型 - 模态状态需要有效的模态类型
    if ((NewState == EMAHUDState::ReviewModal || NewState == EMAHUDState::EditingModal) 
        && ModalType == EMAModalType::None)
    {
        UE_LOG(LogMAHUDState, Error, TEXT("Modal state requires valid ModalType"));
        return false;
    }

    // 记录转换
    EMAHUDState OldState = CurrentState;
    RecordTransition(OldState, NewState, ModalType);

    // 更新状态
    SetState(NewState);
    ActiveModalType = (NewState == EMAHUDState::ReviewModal || NewState == EMAHUDState::EditingModal) 
        ? ModalType 
        : EMAModalType::None;

    // 如果进入模态状态，根据模态类型决定是否清除待处理通知
    // 对于 Decision Modal，不清除通知，因为用户可能只是查看然后关闭窗口

    // 只有在用户点击 Confirm/Reject/Option 按钮时才清除通知
    if (NewState == EMAHUDState::ReviewModal || NewState == EMAHUDState::EditingModal)
    {
        if (ModalType != EMAModalType::Decision)
        {
            // 非 Decision Modal，清除通知
            PendingNotification = EMANotificationType::None;
        }
        // Decision Modal 保留通知，直到用户确认/拒绝
    }

    UE_LOG(LogMAHUDState, Log, TEXT("State transition: %s -> %s (Modal: %s, PendingNotification: %s)"),
        *UEnum::GetValueAsString(OldState),
        *UEnum::GetValueAsString(NewState),
        *UEnum::GetValueAsString(ModalType),
        *UEnum::GetValueAsString(PendingNotification));

    // 触发委托
    OnStateChanged.Broadcast(OldState, NewState);

    return true;
}

void UMAHUDStateManager::ReturnToNormalHUD()
{
    if (CurrentState != EMAHUDState::NormalHUD)
    {
        TransitionToState(EMAHUDState::NormalHUD);
    }
    
    // 处理延迟的通知队列
    ProcessDeferredNotifications();
}

bool UMAHUDStateManager::IsValidTransition(EMAHUDState FromState, EMAHUDState ToState) const
{
    // 相同状态不需要转换
    if (FromState == ToState)
    {
        return false;
    }

    // 定义有效的状态转换
    switch (FromState)
    {
    case EMAHUDState::NormalHUD:
        // 从正常状态可以转换到：通知待处理、查看模态、编辑模态
        return ToState == EMAHUDState::NotificationPending 
            || ToState == EMAHUDState::ReviewModal 
            || ToState == EMAHUDState::EditingModal;

    case EMAHUDState::NotificationPending:
        // 从通知待处理状态可以转换到：正常状态、查看模态、编辑模态
        return ToState == EMAHUDState::NormalHUD 
            || ToState == EMAHUDState::ReviewModal 
            || ToState == EMAHUDState::EditingModal;

    case EMAHUDState::ReviewModal:
        // 从查看模态状态可以转换到：正常状态、编辑模态
        return ToState == EMAHUDState::NormalHUD 
            || ToState == EMAHUDState::EditingModal;

    case EMAHUDState::EditingModal:
        // 从编辑模态状态可以转换到：正常状态、查看模态（Save 返回时）
        return ToState == EMAHUDState::NormalHUD
            || ToState == EMAHUDState::ReviewModal;

    default:
        return false;
    }
}

//=============================================================================
// 通知处理
//=============================================================================

void UMAHUDStateManager::ShowNotification(EMANotificationType Type)
{
    if (Type == EMANotificationType::None)
    {
        UE_LOG(LogMAHUDState, Warning, TEXT("Cannot show notification with type None"));
        return;
    }

    // 如果当前有模态窗口打开，将通知加入延迟队列
    if (IsModalActive())
    {
        // 检查队列中是否已有相同类型的通知
        if (!DeferredNotifications.Contains(Type))
        {
            DeferredNotifications.Add(Type);
            UE_LOG(LogMAHUDState, Log, TEXT("Notification deferred (modal active): %s, queue size: %d"),
                *UEnum::GetValueAsString(Type), DeferredNotifications.Num());
        }
        else
        {
            UE_LOG(LogMAHUDState, Log, TEXT("Notification already in deferred queue: %s"),
                *UEnum::GetValueAsString(Type));
        }
        return;
    }

    PendingNotification = Type;

    // 转换到通知待处理状态（如果还不是的话）
    if (CurrentState == EMAHUDState::NormalHUD)
    {
        TransitionToState(EMAHUDState::NotificationPending);
    }
    // 如果已经是 NotificationPending 状态，不需要转换，但仍然需要广播通知
    // 这样可以更新通知内容（比如从 TaskGraph 通知变为 SkillList 通知）

    // 触发通知委托 - 无论状态是否转换，都要广播以更新 UI
    UE_LOG(LogMAHUDState, Log, TEXT("Broadcasting notification: %s"),
        *UEnum::GetValueAsString(Type));
    OnNotificationReceived.Broadcast(Type);

    UE_LOG(LogMAHUDState, Log, TEXT("Notification shown: %s"),
        *UEnum::GetValueAsString(Type));
}

void UMAHUDStateManager::DismissNotification()
{
    if (PendingNotification != EMANotificationType::None)
    {
        PendingNotification = EMANotificationType::None;

        // 如果当前是通知待处理状态，返回正常状态
        if (CurrentState == EMAHUDState::NotificationPending)
        {
            TransitionToState(EMAHUDState::NormalHUD);
        }

        UE_LOG(LogMAHUDState, Log, TEXT("Notification dismissed"));
    }
}

//=============================================================================
// 输入处理
//=============================================================================

void UMAHUDStateManager::HandleCheckTaskInput()
{
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckTaskInput called, CurrentState: %s, ActiveModal: %s, IsModalActive: %s"),
        *UEnum::GetValueAsString(CurrentState),
        *UEnum::GetValueAsString(ActiveModalType),
        IsModalActive() ? TEXT("true") : TEXT("false"));

    // 如果任务图模态已打开，关闭它
    if (IsModalActive() && ActiveModalType == EMAModalType::TaskGraph)
    {
        UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckTaskInput: TaskGraph modal is active, closing it"));
        ReturnToNormalHUD();
        return;
    }

    // 如果其他模态已打开，忽略输入
    if (IsModalActive())
    {
        UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckTaskInput: Input ignored - different modal is active (ActiveModal: %s)"),
            *UEnum::GetValueAsString(ActiveModalType));
        return;
    }

    // 如果有任务图相关的通知，先清除它
    if (PendingNotification == EMANotificationType::TaskGraphUpdate)
    {
        PendingNotification = EMANotificationType::None;
    }

    // 打开任务图查看模态
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckTaskInput: Attempting to transition to ReviewModal with TaskGraph"));
    bool bSuccess = TransitionToState(EMAHUDState::ReviewModal, EMAModalType::TaskGraph);
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckTaskInput: TransitionToState result: %s"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
}

void UMAHUDStateManager::HandleCheckSkillInput()
{
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckSkillInput called, CurrentState: %s, ActiveModal: %s, IsModalActive: %s"),
        *UEnum::GetValueAsString(CurrentState),
        *UEnum::GetValueAsString(ActiveModalType),
        IsModalActive() ? TEXT("true") : TEXT("false"));

    // 如果技能列表模态已打开，关闭它
    if (IsModalActive() && ActiveModalType == EMAModalType::SkillAllocation)
    {
        UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckSkillInput: SkillList modal is active, closing it"));
        ReturnToNormalHUD();
        return;
    }

    // 如果其他模态已打开，忽略输入
    if (IsModalActive())
    {
        UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckSkillInput: Input ignored - different modal is active (ActiveModal: %s)"),
            *UEnum::GetValueAsString(ActiveModalType));
        return;
    }

    // 如果有技能列表相关的通知，先清除它
    if (PendingNotification == EMANotificationType::SkillListUpdate || 
        PendingNotification == EMANotificationType::SkillListExecuting)
    {
        PendingNotification = EMANotificationType::None;
    }

    // 打开技能列表查看模态
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckSkillInput: Attempting to transition to ReviewModal with SkillList"));
    bool bSuccess = TransitionToState(EMAHUDState::ReviewModal, EMAModalType::SkillAllocation);
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckSkillInput: TransitionToState result: %s"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
}


void UMAHUDStateManager::HandleCheckDecisionInput()
{
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckDecisionInput called, CurrentState: %s, ActiveModal: %s, PendingNotification: %s"),
        *UEnum::GetValueAsString(CurrentState),
        *UEnum::GetValueAsString(ActiveModalType),
        *UEnum::GetValueAsString(PendingNotification));

    // 如果决策模态已打开，只关闭窗口，不清除决策状态
    if (IsModalActive() && ActiveModalType == EMAModalType::Decision)
    {
        UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckDecisionInput: Decision modal is open, closing it (keeping decision state)"));
        EMAHUDState OldState = CurrentState;
        SetState(EMAHUDState::NotificationPending);
        ActiveModalType = EMAModalType::None;
        OnStateChanged.Broadcast(OldState, EMAHUDState::NotificationPending);
        return;
    }

    // 如果其他模态已打开，忽略输入
    if (IsModalActive())
    {
        UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckDecisionInput: Input ignored - different modal is active"));
        return;
    }

    // 检查是否有待处理的决策通知
    if (PendingNotification != EMANotificationType::DecisionUpdate)
    {
        UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckDecisionInput: No pending decision notification, ignoring input"));
        return;
    }

    // 决策直接进入编辑模态（用户需要操作 Yes/No 按钮）
    EMAHUDState OldDecisionState = CurrentState;
    RecordTransition(OldDecisionState, EMAHUDState::EditingModal, EMAModalType::Decision);
    SetState(EMAHUDState::EditingModal);
    ActiveModalType = EMAModalType::Decision;
    
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckDecisionInput: State transition: %s -> EditingModal (Modal: Decision)"),
        *UEnum::GetValueAsString(OldDecisionState));
    
    OnStateChanged.Broadcast(OldDecisionState, EMAHUDState::EditingModal);
}

//=============================================================================
// 模态操作
//=============================================================================

void UMAHUDStateManager::OnModalConfirm()
{
    if (!IsModalActive())
    {
        UE_LOG(LogMAHUDState, Warning, TEXT("OnModalConfirm called but no modal is active"));
        return;
    }

    EMAModalType ConfirmedModalType = ActiveModalType;

    UE_LOG(LogMAHUDState, Log, TEXT("Modal confirmed: %s"),
        *UEnum::GetValueAsString(ConfirmedModalType));

    // 返回正常状态
    ReturnToNormalHUD();

    // 触发确认委托
    OnModalConfirmed.Broadcast(ConfirmedModalType);
}

void UMAHUDStateManager::OnModalReject()
{
    if (!IsModalActive())
    {
        UE_LOG(LogMAHUDState, Warning, TEXT("OnModalReject called but no modal is active"));
        return;
    }

    EMAModalType RejectedModalType = ActiveModalType;

    UE_LOG(LogMAHUDState, Log, TEXT("Modal rejected: %s"),
        *UEnum::GetValueAsString(RejectedModalType));

    // 返回正常状态
    ReturnToNormalHUD();

    // 触发拒绝委托
    OnModalRejected.Broadcast(RejectedModalType);
}

void UMAHUDStateManager::OnModalEdit()
{
    // 只有在查看模态状态下才能切换到编辑模态
    if (CurrentState != EMAHUDState::ReviewModal)
    {
        UE_LOG(LogMAHUDState, Warning, TEXT("OnModalEdit called but not in ReviewModal state"));
        return;
    }

    EMAModalType CurrentModalType = ActiveModalType;

    UE_LOG(LogMAHUDState, Log, TEXT("Modal edit requested: %s"),
        *UEnum::GetValueAsString(CurrentModalType));

    // 转换到编辑模态
    TransitionToState(EMAHUDState::EditingModal, CurrentModalType);

    // 触发编辑请求委托
    OnModalEditRequested.Broadcast(CurrentModalType);
}

//=============================================================================
// 内部方法
//=============================================================================

void UMAHUDStateManager::SetState(EMAHUDState NewState)
{
    CurrentState = NewState;
}

void UMAHUDStateManager::RecordTransition(EMAHUDState FromState, EMAHUDState ToState, EMAModalType ModalType)
{
    FMAHUDStateTransition Transition(FromState, ToState, ModalType);
    TransitionHistory.Add(Transition);

    // 限制历史记录大小
    if (TransitionHistory.Num() > MaxHistorySize)
    {
        TransitionHistory.RemoveAt(0);
    }
}

EMAModalType UMAHUDStateManager::GetModalTypeForNotification(EMANotificationType NotificationType) const
{
    switch (NotificationType)
    {
    case EMANotificationType::TaskGraphUpdate:
        return EMAModalType::TaskGraph;

    case EMANotificationType::SkillListUpdate:
    case EMANotificationType::SkillListExecuting:
        return EMAModalType::SkillAllocation;


    case EMANotificationType::DecisionUpdate:
        return EMAModalType::Decision;

    default:
        return EMAModalType::None;
    }
}

void UMAHUDStateManager::ProcessDeferredNotifications()
{
    // 如果队列为空，直接返回
    if (DeferredNotifications.Num() == 0)
    {
        return;
    }

    // 取出队列中的第一个通知
    EMANotificationType NextNotification = DeferredNotifications[0];
    DeferredNotifications.RemoveAt(0);

    UE_LOG(LogMAHUDState, Log, TEXT("Processing deferred notification: %s, remaining in queue: %d"),
        *UEnum::GetValueAsString(NextNotification), DeferredNotifications.Num());

    // 显示通知 (此时 modal 已关闭，所以不会再被加入队列)
    ShowNotification(NextNotification);
}
