// MARobotDogCharacter.h
// 机器狗角色 - 支持 StateTree AI

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "../Actor/MAPatrolPath.h"
#include "MARobotDogCharacter.generated.h"

class AMAChargingStation;
class UMAStateTreeComponent;

UCLASS()
class MULTIAGENT_API AMARobotDogCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMARobotDogCharacter();

    virtual void Tick(float DeltaTime) override;

    void PlayWalkAnimation();
    void PlayIdleAnimation();

    // ========== Energy System ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float Energy = 100.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float MaxEnergy = 100.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float EnergyDrainRate = 1.f;  // 每秒消耗
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    void DrainEnergy(float DeltaTime);
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    void RestoreEnergy(float Amount);
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    bool HasEnergy() const { return Energy > 0.f; }
    
    UFUNCTION(BlueprintCallable, Category = "Energy")
    float GetEnergyPercent() const { return (MaxEnergy > 0.f) ? (Energy / MaxEnergy * 100.f) : 0.f; }

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
    // ========== Coverage System ==========
    // 机器人扫描半径（用于覆盖任务和跟随距离）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
    float ScanRadius = 200.f;

    // 覆盖区域引用（可动态设置）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
    TWeakObjectPtr<AActor> CoverageArea;

    // 设置覆盖区域
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    void SetCoverageArea(AActor* Area) { CoverageArea = Area; }

    // 获取覆盖区域
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    AActor* GetCoverageArea() const { return CoverageArea.Get(); }

    // ========== Follow System ==========
    // 当前跟随目标（可动态设置）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    TWeakObjectPtr<AMACharacter> FollowTarget;

    // 设置跟随目标
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void SetFollowTarget(AMACharacter* Target) { FollowTarget = Target; }

    // 清除跟随目标
    UFUNCTION(BlueprintCallable, Category = "Follow")
    void ClearFollowTarget() { FollowTarget.Reset(); }

    // 获取跟随目标
    UFUNCTION(BlueprintCallable, Category = "Follow")
    AMACharacter* GetFollowTarget() const { return FollowTarget.Get(); }

    // ========== Patrol System ==========
    // 巡逻路径引用（可动态设置）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    TWeakObjectPtr<AMAPatrolPath> PatrolPath;

    // 设置巡逻路径
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void SetPatrolPath(AMAPatrolPath* Path) { PatrolPath = Path; }

    // 获取巡逻路径
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    AMAPatrolPath* GetPatrolPath() const { return PatrolPath.Get(); }
};
