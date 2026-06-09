// SK_Transport.h
// 运输技能 - 将一个物体运输到指定位置，支持单机器人或多机器人协作
//
// 多机器人协作通过 UMATransportCoordinator（局部协商层）完成：
// - 同一时间步内运输同一物体到同一目的地的机器人组成一个 Session
// - 协调器分配抓取点 / 编队偏移 / leader 角色
// - 所有参与者就位后，物体附着到 leader，编队抬起并运输
// - 到达目的地后保留附着（机器人很可能仍在空中）
//
// 执行阶段:
// 1. Joining        : 加入 Session，等待一帧让同组其它机器人也加入
// 2. MoveToGrasp     : 飞往分配到的抓取点
// 3. WaitReady       : 上报就位，等待全体就位 + 物体抬起
// 4. Ascend          : 抬起后按需垂直爬升到 LiftAltitude（仅飞行器、当前 Z < LiftAltitude 时）
// 5. Carry           : 编队飞往目的地（leader 携带物体）
// 6. Complete        : 完成（保留附着）

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Transport.generated.h"

class UMANavigationService;
class UMATransportCoordinator;
class UMASkillComponent;
class AMACharacter;

/** 运输技能执行阶段 */
UENUM()
enum class ETransportPhase : uint8
{
    Joining,      // 加入 Session，等待定稿
    MoveToGrasp,  // 飞往抓取点
    WaitReady,    // 已就位，等待全体就位与抬起
    Ascend,       // 抬起后按需垂直爬升到 LiftAltitude
    Carry,        // 携带运输到目的地
    Complete      // 完成
};

/**
 * 运输技能
 */
UCLASS()
class MULTIAGENT_API USK_Transport : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Transport();

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

private:
    //=========================================================================
    // 配置 (从 simulation.json 的 transport 段加载)
    //=========================================================================

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float GraspHeightOffset = 15.f;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float LiftAltitude = 1000.f;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float CarryAltitude = 1500.f;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float AcceptanceRadius = 100.f;

    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float ReadyTimeout = 30.f;

    //=========================================================================
    // 状态
    //=========================================================================

    ETransportPhase CurrentPhase = ETransportPhase::Joining;
    bool bTransportSucceeded = false;
    FString TransportResultMessage;
    float StartTime = 0.f;

    bool bIsAircraft = false;
    float MinFlightAltitude = 800.f;

    FString SessionKey;
    FVector Destination = FVector::ZeroVector;
    FVector GraspPoint = FVector::ZeroVector;
    FVector FormationOffset = FVector::ZeroVector;
    bool bIsLeader = false;
    int32 ParticipantCount = 0;

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    //=========================================================================
    // 引用
    //=========================================================================

    UPROPERTY()
    TObjectPtr<UMANavigationService> NavigationService;

    TWeakObjectPtr<AActor> TargetObject;

    /** 等待 / 轮询定时器（用于 Joining 定稿、WaitReady 栅栏同步） */
    FTimerHandle PhaseTimerHandle;

    //=========================================================================
    // 流程
    //=========================================================================

    bool InitializeTransportContext(AMACharacter& Character, UMASkillComponent& SkillComp);
    UMATransportCoordinator* GetCoordinator() const;

    void EnterMoveToGrasp();
    void EnterAscend();
    void EnterCarry();
    void OnReadyTick();        // WaitReady 阶段栅栏轮询
    void CompleteTransport();
    void FailTransport(const FString& ResultMessage, const FString& ErrorReason, const FString& StatusMessage = FString());
    void ResetTransportRuntimeState();

    /** 飞行器航点高度修正 */
    FVector ApplyFlightAltitude(const FVector& Point, float Altitude) const;

    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);

    /** Joining 阶段：延迟一帧定稿后调用 */
    void OnFinalizeTick();
};
