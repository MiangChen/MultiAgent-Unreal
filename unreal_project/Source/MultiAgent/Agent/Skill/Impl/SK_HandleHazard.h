// SK_HandleHazard.h
// 处理危险技能 - 多阶段实现

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_HandleHazard.generated.h"

class UMANavigationService;
class AMAWaterSpray;
class AMAFire;

/** 技能执行阶段 */
UENUM()
enum class EHandleHazardPhase : uint8
{
    MoveToSafeDistance,  // 导航到安全距离
    TurnToTarget,        // 转向目标
    StartSpray,          // 开始喷水
    WaitForExtinguish,   // 等待熄灭
    Complete             // 完成
};

/**
 * 处理危险技能
 * 
 * 执行流程:
 * 1. 导航到危险源安全距离
 * 2. 播放水喷射特效
 * 3. 等待处理完成
 * 4. 熄灭火焰，结束技能
 */
UCLASS()
class MULTIAGENT_API USK_HandleHazard : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_HandleHazard();

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
    // 配置
    //=========================================================================
    
    /** 安全距离 (cm) */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float SafeDistance = 500.f;

    /** 处理持续时间 (秒) */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float HandleDuration = 15.0f;

    /** 喷射速度/范围 (cm) */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float SpraySpeed = 500.f;

    /** 水柱宽度 (cm) */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float SprayWidth = 30.f;

    //=========================================================================
    // 状态
    //=========================================================================
    
    EHandleHazardPhase CurrentPhase = EHandleHazardPhase::MoveToSafeDistance;
    float HandleProgress = 0.f;
    float StartTime = 0.f;
    bool bHandleSucceeded = false;
    FString HandleResultMessage;
    
    /** 是否是飞行机器人 */
    bool bIsAircraft = false;
    
    /** 飞行机器人最小高度限制 (cm) */
    float MinFlightAltitude = 800.f;

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    //=========================================================================
    // 引用
    //=========================================================================
    
    UPROPERTY()
    TObjectPtr<UMANavigationService> NavigationService;

    UPROPERTY()
    TObjectPtr<AMAWaterSpray> WaterSpray;

    /** 目标火焰（情况1：直接是火焰对象） */
    TWeakObjectPtr<AMAFire> TargetFire;
    
    /** 目标环境对象（情况2：着火的环境实体） */
    TWeakObjectPtr<AActor> TargetEnvironmentObject;
    
    /** 喷水目标位置 */
    FVector TargetLocation;
    
    /** 开始喷水时的位置（用于诊断移动问题） */
    FVector SprayStartLocation;
    
    FTimerHandle ProgressTimerHandle;

    //=========================================================================
    // 阶段处理
    //=========================================================================
    
    void UpdatePhase();
    void HandleMoveToSafeDistance();
    void HandleTurnToTarget();
    void HandleStartSpray();
    void HandleWaitForExtinguish();
    void HandleComplete();

    //=========================================================================
    // 回调
    //=========================================================================
    
    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);

    void OnProgressTick();
    void OnTurnTick();

    //=========================================================================
    // 辅助
    //=========================================================================
    
    FVector CalculateSafePosition() const;
    void SpawnWaterSpray();
    void CleanupWaterSpray();
    FString GetPhaseString() const;
    
    /** 转向计时器 */
    FTimerHandle TurnTimerHandle;
};
