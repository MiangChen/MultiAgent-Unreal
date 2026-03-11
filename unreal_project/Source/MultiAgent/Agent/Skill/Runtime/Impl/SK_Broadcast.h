// SK_Broadcast.h
// 喊话技能 - 多阶段实现
// 1. 移动到安全距离
// 2. 转向目标
// 3. 播放喊话特效
// 4. 完成

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Broadcast.generated.h"

class UMANavigationService;
class UTextRenderComponent;
class AMAShockWave;
class AMATextDisplay;

/** 技能执行阶段 */
UENUM()
enum class EBroadcastPhase : uint8
{
    MoveToDistance,   // 移动到喊话距离
    TurnToTarget,     // 转向目标
    Broadcasting,     // 喊话中（播放特效）
    Complete          // 完成
};

UCLASS()
class MULTIAGENT_API USK_Broadcast : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Broadcast();

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
    
    /** 喊话距离 (cm) - 与目标保持的水平距离 */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float BroadcastDistance = 500.f;

    /** 喊话持续时间 (秒) */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float BroadcastDuration = 5.0f;

    /** 特效速度/范围 (cm) - 占位参数，后续替换特效时使用 */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float EffectSpeed = 500.f;

    /** 特效宽度 (cm) - 占位参数 */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float EffectWidth = 30.f;

    /** 声波发射频率 (次/秒) */
    UPROPERTY(EditDefaultsOnly, Category = "Config")
    float EffectRate = 5.0f;

    //=========================================================================
    // 状态
    //=========================================================================
    
    EBroadcastPhase CurrentPhase = EBroadcastPhase::MoveToDistance;
    bool bBroadcastSucceeded = false;
    FString BroadcastResultMessage;
    float BroadcastProgress = 0.f;
    float StartTime = 0.f;
    
    /** 是否是飞行机器人 */
    bool bIsAircraft = false;
    
    /** 飞行机器人最小高度限制 (cm) */
    float MinFlightAltitude = 800.f;

    /** 喊话内容 */
    FString BroadcastMessage;

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    //=========================================================================
    // 引用
    //=========================================================================
    
    UPROPERTY()
    TObjectPtr<UMANavigationService> NavigationService;

    /** 喊话特效 (声波特效) */
    UPROPERTY()
    TObjectPtr<AMAShockWave> BroadcastEffect;

    /** 文本显示屏 */
    UPROPERTY()
    TObjectPtr<AMATextDisplay> TextDisplay;

    /** 文本气泡 (旧版，保留兼容) */
    UPROPERTY()
    TObjectPtr<UTextRenderComponent> TextBubble;

    /** 目标 Actor */
    TWeakObjectPtr<AActor> TargetActor;
    
    /** 目标位置 */
    FVector TargetLocation;
    
    /** 目标名称 */
    FString TargetName;
    
    FTimerHandle BroadcastTimerHandle;
    FTimerHandle TurnTimerHandle;
    FTimerHandle ProgressTimerHandle;

    //=========================================================================
    // 阶段处理
    //=========================================================================
    
    void UpdatePhase();
    void HandleMoveToDistance();
    void HandleTurnToTarget();
    void HandleBroadcasting();
    void HandleComplete();

    //=========================================================================
    // 回调
    //=========================================================================
    
    UFUNCTION()
    void OnNavigationCompleted(bool bSuccess, const FString& Message);

    void OnTurnTick();
    void OnProgressTick();

    //=========================================================================
    // 辅助
    //=========================================================================
    
    FVector CalculateBroadcastPosition() const;
    void SpawnBroadcastEffect();
    void CleanupBroadcastEffect();
    void ShowTextBubble(const FString& Message);
    void HideTextBubble();
    FString GetPhaseString() const;
};
