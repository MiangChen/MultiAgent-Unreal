// SK_Place.h
// Place 技能 - 搬运物体到目标位置
// 支持三种模式：装货到UGV、从UGV卸货到地面、堆叠物体

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "SK_Place.generated.h"

class AMAPickupItem;
class AMAUGVCharacter;
class AMAHumanoidCharacter;

// Place 操作模式
UENUM(BlueprintType)
enum class EPlaceMode : uint8
{
    LoadToUGV,        // 装货到 UGV
    UnloadToGround,   // 从 UGV 卸货到地面
    StackOnObject     // 堆叠到另一个物体
};

// Place 执行阶段（扩展状态机）
UENUM(BlueprintType)
enum class EPlacePhase : uint8
{
    MoveToSource,      // 移动到源对象（target 或 UGV）
    BendDownPickup,    // 俯身拾取
    StandUpWithItem,   // 起身（持有物体）
    MoveToTarget,      // 移动到目标（UGV、地面或 surface_target）
    BendDownPlace,     // 俯身放置
    StandUpEmpty,      // 起身（空手）
    Complete           // 完成
};

UCLASS()
class MULTIAGENT_API USK_Place : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Place();

    UPROPERTY(EditDefaultsOnly, Category = "Place")
    float InteractionRadius = 150.f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Place")
    float BendDuration = 0.8f;
    
    /** 获取当前操作模式 */
    UFUNCTION(BlueprintPure, Category = "Place")
    EPlaceMode GetPlaceMode() const { return CurrentMode; }
    
    /** 获取当前执行阶段 */
    UFUNCTION(BlueprintPure, Category = "Place")
    EPlacePhase GetCurrentPhase() const { return CurrentPhase; }

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    // ========== 状态机 ==========
    void UpdatePhase();
    
    // ========== 阶段处理 ==========
    void HandleMoveToSource();
    void HandleBendDownPickup();
    void HandleStandUpWithItem();
    void HandleMoveToTarget();
    void HandleBendDownPlace();
    void HandleStandUpEmpty();
    void HandleComplete();
    
    // ========== 物体操作 ==========
    void PerformPickup();
    void PerformPickupFromUGV();
    void PerformPlaceOnGround();
    void PerformPlaceOnUGV();
    void PerformPlaceOnObject();
    
    // ========== 动画回调 ==========
    UFUNCTION()
    void OnBendDownComplete();
    
    UFUNCTION()
    void OnStandUpComplete();
    
    // ========== 辅助方法 ==========
    AMAHumanoidCharacter* GetHumanoidCharacter() const;
    FString GetPhaseString() const;
    
    // ========== 缓存数据 ==========
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    
    // ========== 状态 ==========
    EPlaceMode CurrentMode = EPlaceMode::LoadToUGV;
    EPlacePhase CurrentPhase = EPlacePhase::MoveToSource;
    FTimerHandle PhaseTimerHandle;
    
    // ========== 目标引用 ==========
    UPROPERTY()
    TWeakObjectPtr<AActor> SourceObject;      // 源对象（要拾取的物体）
    
    UPROPERTY()
    TWeakObjectPtr<AActor> TargetObject;      // 目标对象（UGV 或 堆叠目标物体）
    
    UPROPERTY()
    TWeakObjectPtr<AActor> HeldObject;        // 当前持有的物体
    
    UPROPERTY()
    TWeakObjectPtr<AMAUGVCharacter> TargetUGV; // 目标 UGV（装货/卸货模式）
    
    // ========== 位置 ==========
    FVector SourceLocation;    // 源对象位置
    FVector TargetLocation;    // 目标位置
    FVector DropLocation;      // 放置位置（地面模式）
    
    // ========== 结果缓存 ==========
    bool bPlaceSucceeded = false;
    FString PlaceResultMessage;
};
