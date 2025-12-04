// MARobotDogCharacter.h
// 机器狗角色 - 支持 StateTree AI

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MARobotDogCharacter.generated.h"

class AMAPatrolPath;
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
};
