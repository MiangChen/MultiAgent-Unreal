// MARobotDogCharacter.h
// 机器狗角色 - 支持 StateTree AI
// 使用 Capability Component 模式管理能力参数

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MARobotDogCharacter.generated.h"

class AMAChargingStation;
class UMAStateTreeComponent;
class UMAEnergyComponent;
class UMAPatrolComponent;
class UMAFollowComponent;
class UMACoverageComponent;

UCLASS()
class MULTIAGENT_API AMARobotDogCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMARobotDogCharacter();

    virtual void Tick(float DeltaTime) override;

    void PlayWalkAnimation();
    void PlayIdleAnimation();

    // ========== Capability Components ==========
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    UMAEnergyComponent* EnergyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    UMAPatrolComponent* PatrolComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    UMAFollowComponent* FollowComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    UMACoverageComponent* CoverageComponent;

    // ========== Robot Abilities ==========
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TrySearch();
    
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void StopSearch();
    
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryCharge();

    // ========== StateTree ==========
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

    // ========== 便捷访问方法 (委托给 Component) ==========
    UFUNCTION(BlueprintCallable, Category = "Energy")
    float GetEnergy() const;

    UFUNCTION(BlueprintCallable, Category = "Energy")
    bool HasEnergy() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY()
    UAnimSequence* IdleAnim;
    
    UPROPERTY()
    UAnimSequence* WalkAnim;
    
    bool bIsPlayingWalk;

public:
    // ========== Avoidance System ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float AvoidanceRadius = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float AvoidanceStrength = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float AvoidanceCheckInterval = 0.1f;

private:
    // ========== 卡住自动跳跃 ==========
    float StuckTime = 0.f;
    float StuckThreshold = 0.5f;
    float JumpCooldown = 0.f;
    
    void CheckStuckAndJump(float DeltaTime);

    // 能量耗尽回调
    UFUNCTION()
    void OnEnergyDepleted();
};
