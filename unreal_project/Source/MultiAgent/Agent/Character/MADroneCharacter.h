// MADroneCharacter.h
// 无人机角色基类 - 支持 3D 飞行、悬停、StateTree AI
// 子类: MADronePhantom4Character, MADroneInspire2Character

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAPatrolPath.h"
#include "../Interface/MAAgentInterfaces.h"
#include "MADroneCharacter.generated.h"

class UMAStateTreeComponent;

UCLASS(Abstract)
class MULTIAGENT_API AMADroneCharacter : public AMACharacter,
    public IMAPatrollable,
    public IMAFollowable,
    public IMACoverable,
    public IMAChargeable
{
    GENERATED_BODY()

public:
    AMADroneCharacter();

    virtual void Tick(float DeltaTime) override;

    // ========== IMAChargeable Interface ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float Energy = 100.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float MaxEnergy = 100.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float EnergyDrainRate = 0.5f;  // 每秒消耗 0.5%
    
    virtual void DrainEnergy(float DeltaTime) override;
    virtual void RestoreEnergy(float Amount) override;
    virtual bool HasEnergy() const override { return Energy > 0.f; }
    virtual float GetEnergy() const override { return Energy; }
    virtual float GetMaxEnergy() const override { return MaxEnergy; }
    virtual float GetEnergyPercent() const override { return (MaxEnergy > 0.f) ? (Energy / MaxEnergy * 100.f) : 0.f; }

    // ========== Flight System ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float FlightAltitude = 75.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float MaxFlightSpeed = 100.f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    bool bIsHovering = false;
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void TakeOff(float TargetAltitude = -1.f);
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void Land();
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void Hover();
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool FlyTo(FVector Destination);

    // ========== Drone Abilities ==========
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TrySearch();
    
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    void StopSearch();
    
    UFUNCTION(BlueprintCallable, Category = "Abilities")
    bool TryCharge();

    // ========== StateTree ==========
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

    // ========== IMAFollowable Interface ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
    TWeakObjectPtr<AMACharacter> FollowTarget;

    virtual void SetFollowTarget(AMACharacter* Target) override { FollowTarget = Target; }
    virtual void ClearFollowTarget() override { FollowTarget.Reset(); }
    virtual AMACharacter* GetFollowTarget() const override { return FollowTarget.Get(); }

    // ========== IMACoverable Interface ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
    float ScanRadius = 300.f;  // 无人机扫描范围更大

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
    TWeakObjectPtr<AActor> CoverageAreaRef;

    virtual void SetCoverageArea(AActor* Area) override { CoverageAreaRef = Area; }
    virtual AActor* GetCoverageArea() const override { return CoverageAreaRef.Get(); }
    virtual float GetScanRadius() const override { return ScanRadius; }

    // ========== IMAPatrollable Interface ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    TWeakObjectPtr<AMAPatrolPath> PatrolPath;

    virtual void SetPatrolPath(AMAPatrolPath* Path) override { PatrolPath = Path; }
    virtual AMAPatrolPath* GetPatrolPath() const override { return PatrolPath.Get(); }

protected:
    // 子类实现：设置 Mesh、动画、碰撞体大小
    virtual void SetupDroneAssets() PURE_VIRTUAL(AMADroneCharacter::SetupDroneAssets, );

    virtual void BeginPlay() override;
    
    void UpdateEnergyDisplay();
    void CheckLowEnergyStatus();
    void UpdatePropellerAnimation();

    UPROPERTY()
    UAnimSequence* PropellerAnim;
    
private:
    static constexpr float LowEnergyThreshold = 20.f;
    
    // 飞行状态
    bool bIsFlying = false;
    FVector TargetFlightLocation;
};
