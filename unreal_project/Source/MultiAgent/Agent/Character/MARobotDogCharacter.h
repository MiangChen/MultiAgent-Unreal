// MARobotDogCharacter.h
// 机器狗角色 - 支持 StateTree AI

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAPatrolPath.h"
#include "../Interface/MAAgentInterfaces.h"
#include "MARobotDogCharacter.generated.h"

class AMAChargingStation;
class UMAStateTreeComponent;

UCLASS()
class MULTIAGENT_API AMARobotDogCharacter : public AMACharacter,
    public IMAPatrollable,
    public IMAFollowable,
    public IMACoverable,
    public IMAChargeable
{
    GENERATED_BODY()

public:
    AMARobotDogCharacter();

    virtual void Tick(float DeltaTime) override;

    void PlayWalkAnimation();
    void PlayIdleAnimation();

    // ========== IMAChargeable Interface ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float Energy = 100.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float MaxEnergy = 100.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float EnergyDrainRate = 1.f;  // 每秒消耗
    
    virtual void DrainEnergy(float DeltaTime) override;
    virtual void RestoreEnergy(float Amount) override;
    virtual bool HasEnergy() const override { return Energy > 0.f; }
    virtual float GetEnergy() const override { return Energy; }
    virtual float GetMaxEnergy() const override { return MaxEnergy; }
    virtual float GetEnergyPercent() const override { return (MaxEnergy > 0.f) ? (Energy / MaxEnergy * 100.f) : 0.f; }

    // ========== Robot Abilities ==========
    // 注意: TryPatrol/TryPatrolPath/StopPatrol 已移除，Patrol 改用 StateTree
    
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TrySearch();
    
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void StopSearch();
    
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryCharge();

    // ========== StateTree ==========
    // StateTree 组件 - 支持运行时动态加载 StateTree Asset
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

protected:
    virtual void BeginPlay() override;
    
    // 更新头顶电量显示
    void UpdateEnergyDisplay();
    
    // 检查低电量状态
    void CheckLowEnergyStatus();

    UPROPERTY()
    UAnimSequence* IdleAnim;
    
    UPROPERTY()
    UAnimSequence* WalkAnim;
    
    bool bIsPlayingWalk;
    
private:
    // 低电量阈值
    static constexpr float LowEnergyThreshold = 20.f;

public:
    // ========== IMACoverable Interface ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
    float ScanRadius = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
    TWeakObjectPtr<AActor> CoverageAreaRef;

    virtual void SetCoverageArea(AActor* Area) override { CoverageAreaRef = Area; }
    virtual AActor* GetCoverageArea() const override { return CoverageAreaRef.Get(); }
    virtual float GetScanRadius() const override { return ScanRadius; }

    // ========== IMAFollowable Interface ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    TWeakObjectPtr<AMACharacter> FollowTarget;

    virtual void SetFollowTarget(AMACharacter* Target) override { FollowTarget = Target; }
    virtual void ClearFollowTarget() override { FollowTarget.Reset(); }
    virtual AMACharacter* GetFollowTarget() const override { return FollowTarget.Get(); }

    // ========== IMAPatrollable Interface ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    TWeakObjectPtr<AMAPatrolPath> PatrolPath;

    virtual void SetPatrolPath(AMAPatrolPath* Path) override { PatrolPath = Path; }
    virtual AMAPatrolPath* GetPatrolPath() const override { return PatrolPath.Get(); }

    // ========== Avoidance System ==========
    // 避障检测半径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float AvoidanceRadius = 100.f;

    // 避障力度 (0-2, 1为正常)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float AvoidanceStrength = 1.f;

    // 避障检测间隔（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float AvoidanceCheckInterval = 0.1f;
};
