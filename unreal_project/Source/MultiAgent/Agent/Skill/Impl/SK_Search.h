// SK_Search.h
// 搜索技能 - 在指定区域内搜索目标

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Search.generated.h"

UCLASS()
class MULTIAGENT_API USK_Search : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Search();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    // 生成割草机式搜索航线
    void GenerateSearchPath();
    
    // 平滑移动更新
    void UpdateSearch();
    
    // 导航到下一个航点（保留兼容）
    void NavigateToNextWaypoint();
    
    // 航点到达回调（保留兼容）
    void OnWaypointReached();

private:
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    // 搜索航线
    TArray<FVector> SearchPath;
    int32 CurrentWaypointIndex = 0;
    
    // 搜索参数
    UPROPERTY()
    float ScanWidth = 200.f;  // 扫描宽度
    
    FTimerHandle UpdateTimerHandle;
    FTimerHandle CheckTimerHandle;
    
    // 结果缓存（用于 EndAbility 生成反馈消息）
    bool bSearchSucceeded = false;
    FString SearchResultMessage;
};
