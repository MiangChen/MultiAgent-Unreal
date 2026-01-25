// SK_Navigate.h
// 导航技能

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "../../../Utils/MAPathPlanner.h"
#include "SK_Navigate.generated.h"

UCLASS()
class MULTIAGENT_API USK_Navigate : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Navigate();

    UFUNCTION(BlueprintCallable, Category = "Navigate")
    void SetTargetLocation(FVector InTargetLocation);

    UPROPERTY(EditDefaultsOnly, Category = "Navigate")
    float AcceptanceRadius = 200.f;

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

private:
    FVector TargetLocation;
    void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
    void StartNavigation();
    void CleanupNavigation();
    void CheckFlightArrival();
    
    // 手动导航（NavMesh 失败时的备选方案）
    void StartManualNavigation();
    void UpdateManualNavigation();

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    FTimerHandle FlightCheckTimerHandle;
    FTimerHandle ManualNavTimerHandle;
    
    // 导航结果缓存（用于 EndAbility 生成反馈消息）
    bool bNavigationSucceeded = false;
    FString NavigationResultMessage;
    
    // 当前导航请求 ID（用于区分不同的导航请求）
    FAIRequestID CurrentRequestID;
    bool bHasActiveRequest = false;
    
    // 手动导航状态
    bool bUsingManualNavigation = false;
    float ManualNavStuckTime = 0.f;
    FVector LastManualNavLocation;
    
    /** 路径规划器实例 */
    TUniquePtr<IMAPathPlanner> PathPlanner;
    
    // 飞行检查日志时间（避免刷屏）
    float LastFlightCheckLogTime = 0.f;
};
