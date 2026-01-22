// SK_Search.h
// 搜索技能 - 在指定区域内搜索目标

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
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
    
    // 平滑移动更新（保留兼容，已改用 TickSearch）
    void UpdateSearch();
    
    // Tick 更新（使用引擎 Tick，更平滑）
    bool TickSearch(float DeltaTime);
    
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
    int32 LastUIUpdateIndex = -1;  // 上次 UI 更新时的航点索引
    
    // 搜索参数
    UPROPERTY()
    float ScanWidth = 1600.f;  // 扫描宽度（默认 16m）
    
    // 提前结束相关
    TArray<FVector> FoundTargetLocations;  // 已找到的目标位置
    bool bHasFoundTarget = false;  // 是否有找到目标
    
    FTimerHandle UpdateTimerHandle;
    FTimerHandle CheckTimerHandle;
    
    // Ticker 句柄（用于平滑更新）
    FTSTicker::FDelegateHandle TickDelegateHandle;
    
    // 结果缓存（用于 EndAbility 生成反馈消息）
    bool bSearchSucceeded = false;
    FString SearchResultMessage;
};
