// SK_Guide.h
// 引导技能 - 引导目标对象到达指定位置

#pragma once

#include "CoreMinimal.h"
#include "../MASkillBase.h"
#include "Containers/Ticker.h"
#include "SK_Guide.generated.h"

class UMANavigationService;

/** 被引导对象的移动模式 */
UENUM(BlueprintType)
enum class EMAGuideTargetMoveMode : uint8
{
    Direct,     // 直接移动 (AddMovementInput)，无避障
    NavMesh     // 导航服务 (NavMesh)，有避障
};

UCLASS()
class MULTIAGENT_API USK_Guide : public UMASkillBase
{
    GENERATED_BODY()

public:
    USK_Guide();

    UPROPERTY(EditDefaultsOnly, Category = "Guide")
    float FollowDistance = 300.f;

    UPROPERTY(EditDefaultsOnly, Category = "Guide")
    float AcceptanceRadius = 200.f;

    UPROPERTY(EditDefaultsOnly, Category = "Guide")
    float TargetAcceptanceRadius = 200.f;

    /** 被引导对象移动模式 (从配置加载) */
    UPROPERTY(EditDefaultsOnly, Category = "Guide")
    EMAGuideTargetMoveMode TargetMoveMode = EMAGuideTargetMoveMode::NavMesh;

    /** 等待距离阈值 - 机器人与被引导对象距离超过此值时停下等待 (从配置加载) */
    UPROPERTY(EditDefaultsOnly, Category = "Guide")
    float WaitDistanceThreshold = 500.f;

    UFUNCTION(BlueprintPure, Category = "Guide")
    float GetGuideProgress() const;

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
    FTSTicker::FDelegateHandle TickDelegateHandle;
    TWeakObjectPtr<AActor> GuideTargetActor;
    FVector GuideDestination;
    FVector StartLocation;
    float StartTime = 0.f;

    FGameplayAbilitySpecHandle CachedHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;

    UPROPERTY()
    UMANavigationService* NavigationService = nullptr;

    UPROPERTY()
    UMANavigationService* TargetNavigationService = nullptr;

    FVector LastTargetNavPos = FVector::ZeroVector;

    bool bAgentArrived = false;
    bool bAgentWaiting = false;  // 机器人是否正在等待被引导对象
    bool bGuideSucceeded = false;
    FString GuideResultMessage;
    float TargetMoveSpeed = 0.f;
    float MinFlightAltitude = 800.f;

    static constexpr float NavUpdateThreshold = 200.f;

    bool TickGuide(float DeltaTime);
    void UpdateTargetPosition(float DeltaTime);
    void UpdateTargetPositionDirect(float DeltaTime, const FVector& TargetFollowPos);
    void UpdateTargetPositionNavMesh(const FVector& TargetFollowPos);
    FVector CalculateTargetFollowPosition() const;

    /** 检查是否需要等待被引导对象，并控制机器人移动 */
    void CheckAndHandleWaiting();

    UFUNCTION()
    void OnAgentNavigationCompleted(bool bSuccess, const FString& Message);

    void OnGuideComplete(bool bSuccess, const FString& Message);
    bool IsFlying() const;
    float GetTargetMoveSpeed() const;
    UMANavigationService* GetTargetNavigationService() const;
};
