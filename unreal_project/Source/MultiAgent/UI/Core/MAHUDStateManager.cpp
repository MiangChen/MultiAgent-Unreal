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

    // 如果进入模态状态，清除待处理通知
    if (NewState == EMAHUDState::ReviewModal || NewState == EMAHUDState::EditingModal)
    {
        PendingNotification = EMANotificationType::None;
    }

    UE_LOG(LogMAHUDState, Log, TEXT("State transition: %s -> %s (Modal: %s)"),
        *UEnum::GetValueAsString(OldState),
        *UEnum::GetValueAsString(NewState),
        *UEnum::GetValueAsString(ModalType));

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
        // 从编辑模态状态只能转换到：正常状态
        return ToState == EMAHUDState::NormalHUD;

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

    // 如果当前有模态窗口打开，不显示通知
    if (IsModalActive())
    {
        UE_LOG(LogMAHUDState, Log, TEXT("Notification ignored - modal is active"));
        return;
    }

    PendingNotification = Type;

    // 转换到通知待处理状态
    if (CurrentState == EMAHUDState::NormalHUD)
    {
        TransitionToState(EMAHUDState::NotificationPending);
    }

    // 触发通知委托
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
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckTaskInput called, CurrentState: %s, ActiveModal: %s"),
        *UEnum::GetValueAsString(CurrentState),
        *UEnum::GetValueAsString(ActiveModalType));

    // 如果任务图模态已打开，关闭它 (Requirements: 10.5)
    if (IsModalActive() && ActiveModalType == EMAModalType::TaskGraph)
    {
        ReturnToNormalHUD();
        return;
    }

    // 如果其他模态已打开，忽略输入
    if (IsModalActive())
    {
        UE_LOG(LogMAHUDState, Log, TEXT("Input ignored - different modal is active"));
        return;
    }

    // 打开任务图查看模态
    TransitionToState(EMAHUDState::ReviewModal, EMAModalType::TaskGraph);
}

void UMAHUDStateManager::HandleCheckSkillInput()
{
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckSkillInput called, CurrentState: %s, ActiveModal: %s"),
        *UEnum::GetValueAsString(CurrentState),
        *UEnum::GetValueAsString(ActiveModalType));

    // 如果技能列表模态已打开，关闭它 (Requirements: 10.5)
    if (IsModalActive() && ActiveModalType == EMAModalType::SkillList)
    {
        ReturnToNormalHUD();
        return;
    }

    // 如果其他模态已打开，忽略输入
    if (IsModalActive())
    {
        UE_LOG(LogMAHUDState, Log, TEXT("Input ignored - different modal is active"));
        return;
    }

    // 打开技能列表查看模态
    TransitionToState(EMAHUDState::ReviewModal, EMAModalType::SkillList);
}

void UMAHUDStateManager::HandleCheckEmergencyInput()
{
    UE_LOG(LogMAHUDState, Log, TEXT("HandleCheckEmergencyInput called, CurrentState: %s, ActiveModal: %s"),
        *UEnum::GetValueAsString(CurrentState),
        *UEnum::GetValueAsString(ActiveModalType));

    // 如果突发事件模态已打开，关闭它 (Requirements: 10.5)
    if (IsModalActive() && ActiveModalType == EMAModalType::Emergency)
    {
        ReturnToNormalHUD();
        return;
    }

    // 如果其他模态已打开，忽略输入
    if (IsModalActive())
    {
        UE_LOG(LogMAHUDState, Log, TEXT("Input ignored - different modal is active"));
        return;
    }

    // 突发事件直接进入编辑模态 (Requirements: 6.2)
    TransitionToState(EMAHUDState::EditingModal, EMAModalType::Emergency);
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

    // 返回正常状态 (Requirements: 2.5, 7.2)
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

    // 返回正常状态 (Requirements: 2.5, 6.6, 7.2)
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

    // 转换到编辑模态 (Requirements: 6.1)
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
        return EMAModalType::SkillList;

    case EMANotificationType::EmergencyEvent:
        return EMAModalType::Emergency;

    default:
        return EMAModalType::None;
    }
}
