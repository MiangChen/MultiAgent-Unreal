// GA_Formation.h
// 编队技能 - 多机器人按阵型移动

#pragma once

#include "CoreMinimal.h"
#include "../MAGameplayAbilityBase.h"
#include "GA_Formation.generated.h"

class AMACharacter;

// 编队类型
UENUM(BlueprintType)
enum class EFormationType : uint8
{
    Line,       // 一字排开
    Column,     // 纵队
    Wedge,      // 楔形 (V形)
    Diamond,    // 菱形 (X)
    Circle      // 圆形
};

UCLASS()
class MULTIAGENT_API UGA_Formation : public UMAGameplayAbilityBase
{
    GENERATED_BODY()

public:
    UGA_Formation();

    // 设置编队参数
    UFUNCTION(BlueprintCallable, Category = "Formation")
    void SetFormation(AMACharacter* InLeader, EFormationType InType, int32 InPosition, int32 InTotalCount = 8);

    // 编队间距
    UPROPERTY(EditDefaultsOnly, Category = "Formation")
    float FormationSpacing = 200.f;

    // 更新间隔
    UPROPERTY(EditDefaultsOnly, Category = "Formation")
    float UpdateInterval = 0.3f;

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

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

private:
    // 编队 Leader
    UPROPERTY()
    TWeakObjectPtr<AMACharacter> Leader;
    
    // 编队类型
    EFormationType FormationType = EFormationType::Line;
    
    // 在编队中的位置索引
    int32 FormationPosition = 0;
    
    // 编队中的总机器人数量（用于 Circle 半径计算）
    int32 TotalRobotCount = 8;
    
    // 定时器句柄
    FTimerHandle FormationTimerHandle;
    
    // 缓存
    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    // 更新编队位置
    void UpdateFormationPosition();
    
    // 计算编队偏移
    FVector CalculateFormationOffset() const;
    
    // 清理
    void CleanupFormation();
};
