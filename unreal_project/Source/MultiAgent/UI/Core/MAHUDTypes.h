// MAHUDTypes.h
// HUD 状态类型定义 - 定义 HUD 状态机相关的枚举和数据结构
// 用于 HUD 管理器的状态管理和 UI 流程控制

#pragma once

#include "CoreMinimal.h"
#include "MAHUDTypes.generated.h"

//=============================================================================
// HUD 状态枚举 (Requirements: 2.1)
//=============================================================================

/**
 * HUD 状态枚举
 * 
 * 定义 HUD 管理器的四种主要状态：
 * - NormalHUD: 正常 HUD 状态，显示小地图和右侧边栏
 * - NotificationPending: 通知待处理状态，显示通知提示
 * - ReviewModal: 查看模态状态，显示只读模态窗口
 * - EditingModal: 编辑模态状态，显示可编辑模态窗口
 */
UENUM(BlueprintType)
enum class EMAHUDState : uint8
{
    /** 正常 HUD 状态 - 显示小地图和右侧边栏 */
    NormalHUD           UMETA(DisplayName = "Normal HUD"),
    
    /** 通知待处理状态 - 显示通知提示，等待用户按键 */
    NotificationPending UMETA(DisplayName = "Notification Pending"),
    
    /** 查看模态状态 - 显示只读模态窗口 */
    ReviewModal         UMETA(DisplayName = "Review Modal"),
    
    /** 编辑模态状态 - 显示可编辑模态窗口 */
    EditingModal        UMETA(DisplayName = "Editing Modal")
};

//=============================================================================
// 模态类型枚举 (Requirements: 2.1)
//=============================================================================

/**
 * 模态窗口类型枚举
 * 
 * 定义可显示的模态窗口类型：
 * - None: 无模态窗口
 * - TaskGraph: 任务图查看/编辑模态
 * - SkillAllocation: 技能列表查看/编辑模态
 * - Emergency: 突发事件处理模态
 */
UENUM(BlueprintType)
enum class EMAModalType : uint8
{
    /** 无模态窗口 */
    None        UMETA(DisplayName = "None"),
    
    /** 任务图模态 - 用于查看和编辑任务 DAG */
    TaskGraph   UMETA(DisplayName = "Task Graph"),
    
    /** 技能列表模态 - 用于查看和编辑技能分配 */
    SkillAllocation   UMETA(DisplayName = "Skill Allocation"),
    
    /** 突发事件模态 - 用于处理紧急情况 */
    Emergency   UMETA(DisplayName = "Emergency")
};

//=============================================================================
// 通知类型枚举 (Requirements: 2.1)
//=============================================================================

/**
 * 通知类型枚举
 * 
 * 定义后端发送的通知类型：
 * - None: 无通知
 * - TaskGraphUpdate: 任务图更新通知
 * - SkillListUpdate: 技能列表更新通知
 * - EmergencyEvent: 突发事件通知
 * - RequestUserCommand: 索要用户指令通知
 */
UENUM(BlueprintType)
enum class EMANotificationType : uint8
{
    /** 无通知 */
    None            UMETA(DisplayName = "None"),
    
    /** 任务图更新通知 - 后端发送了新的任务图 */
    TaskGraphUpdate UMETA(DisplayName = "Task Graph Update"),
    
    /** 技能列表更新通知 - 后端发送了新的技能分配 (需要用户手动执行) */
    SkillListUpdate UMETA(DisplayName = "Skill List Update"),
    
    /** 技能列表执行中通知 - 后端发送了可执行的技能列表，正在自动执行 */
    SkillListExecuting UMETA(DisplayName = "Skill List Executing"),
    
    /** 突发事件通知 - 后端报告了紧急情况 */
    EmergencyEvent  UMETA(DisplayName = "Emergency Event"),
    
    /** 索要用户指令通知 - 后端请求用户输入指令 */
    RequestUserCommand  UMETA(DisplayName = "Request User Command")
};

//=============================================================================
// 状态转换数据结构
//=============================================================================

/**
 * HUD 状态转换数据
 * 
 * 记录状态转换的详细信息，用于调试和日志
 */
USTRUCT(BlueprintType)
struct FMAHUDStateTransition
{
    GENERATED_BODY()

    /** 转换前的状态 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    EMAHUDState FromState = EMAHUDState::NormalHUD;

    /** 转换后的状态 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    EMAHUDState ToState = EMAHUDState::NormalHUD;

    /** 关联的模态类型 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    EMAModalType ModalType = EMAModalType::None;

    /** 转换动画时长 (秒) */
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float TransitionDuration = 0.2f;

    /** 转换时间戳 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    FDateTime Timestamp;

    FMAHUDStateTransition()
        : Timestamp(FDateTime::Now())
    {
    }

    FMAHUDStateTransition(EMAHUDState InFromState, EMAHUDState InToState, EMAModalType InModalType = EMAModalType::None)
        : FromState(InFromState)
        , ToState(InToState)
        , ModalType(InModalType)
        , Timestamp(FDateTime::Now())
    {
    }
};

//=============================================================================
// 通知数据结构
//=============================================================================

/**
 * 通知数据
 * 
 * 存储通知的详细信息
 */
USTRUCT(BlueprintType)
struct FMANotificationData
{
    GENERATED_BODY()

    /** 通知类型 */
    UPROPERTY(BlueprintReadOnly, Category = "Notification")
    EMANotificationType Type = EMANotificationType::None;

    /** 通知消息 */
    UPROPERTY(BlueprintReadOnly, Category = "Notification")
    FString Message;

    /** 按键提示 */
    UPROPERTY(BlueprintReadOnly, Category = "Notification")
    FString KeyHint;

    /** 通知时间戳 */
    UPROPERTY(BlueprintReadOnly, Category = "Notification")
    FDateTime Timestamp;

    FMANotificationData()
        : Timestamp(FDateTime::Now())
    {
    }

    FMANotificationData(EMANotificationType InType, const FString& InMessage, const FString& InKeyHint)
        : Type(InType)
        , Message(InMessage)
        , KeyHint(InKeyHint)
        , Timestamp(FDateTime::Now())
    {
    }
};
