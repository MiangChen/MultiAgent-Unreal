// MADroneCharacter.h
// 无人机角色基类 - 支持 3D 飞行、悬停、StateTree AI
// 子类: MADronePhantom4Character, MADroneInspire2Character
//
// 飞行系统说明:
// - 不使用 NavMesh，采用直接位置控制
// - 支持碰撞检测，便于未来开发避障算法
// - 状态机: Landed → TakeOff → Hovering ↔ Flying → Land → Landed

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAPatrolPath.h"
#include "../Interface/MAAgentInterfaces.h"
#include "MADroneCharacter.generated.h"

class UMAStateTreeComponent;

// 无人机飞行状态
UENUM(BlueprintType)
enum class EMADroneFlightState : uint8
{
    Landed      UMETA(DisplayName = "Landed"),      // 停在地面
    TakingOff   UMETA(DisplayName = "TakingOff"),   // 正在起飞
    Hovering    UMETA(DisplayName = "Hovering"),    // 悬停中
    Flying      UMETA(DisplayName = "Flying"),      // 飞行中
    Landing     UMETA(DisplayName = "Landing")      // 正在降落
};

// 飞行完成委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlightCompleted, bool, bSuccess, FVector, FinalLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCollisionDetected, FHitResult, HitResult);

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
    float EnergyDrainRate = 0.5f;  // 每秒消耗
    
    virtual void DrainEnergy(float DeltaTime) override;
    virtual void RestoreEnergy(float Amount) override;
    virtual bool HasEnergy() const override { return Energy > 0.f; }
    virtual float GetEnergy() const override { return Energy; }
    virtual float GetMaxEnergy() const override { return MaxEnergy; }
    virtual float GetEnergyPercent() const override { return (MaxEnergy > 0.f) ? (Energy / MaxEnergy * 100.f) : 0.f; }

    // ========== Flight System ==========
    
    // 飞行参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float DefaultFlightAltitude = 500.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float MaxFlightSpeed = 400.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float FlightAcceleration = 200.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float AcceptanceRadius = 100.f;
    
    // 碰撞检测参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision")
    float CollisionCheckDistance = 300.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision")
    float CollisionCheckRadius = 50.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Collision")
    bool bEnableCollisionCheck = true;
    
    // 当前状态
    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    EMADroneFlightState FlightState = EMADroneFlightState::Landed;
    
    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    FVector CurrentFlightTarget;
    
    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    float CurrentSpeed = 0.f;
    
    // ========== 飞行控制函数 ==========
    
    // 起飞到指定高度 (默认使用 DefaultFlightAltitude)
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool TakeOff(float TargetAltitude = -1.f);
    
    // 降落到地面
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool Land();
    
    // 悬停在当前位置
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void Hover();
    
    // 飞向目标位置 (不使用 NavMesh，直接飞行)
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool FlyTo(FVector Destination);
    
    // 取消当前飞行任务
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void CancelFlight();
    
    // 查询函数
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool IsFlying() const { return FlightState == EMADroneFlightState::Flying; }
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool IsHovering() const { return FlightState == EMADroneFlightState::Hovering; }
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool IsLanded() const { return FlightState == EMADroneFlightState::Landed; }
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool IsInAir() const { return FlightState != EMADroneFlightState::Landed; }
    
    // ========== 碰撞检测 ==========
    
    // 检测前方是否有障碍物
    UFUNCTION(BlueprintCallable, Category = "Flight|Collision")
    bool CheckForwardCollision(FHitResult& OutHit) const;
    
    // 检测指定方向是否有障碍物
    UFUNCTION(BlueprintCallable, Category = "Flight|Collision")
    bool CheckCollisionInDirection(FVector Direction, float Distance, FHitResult& OutHit) const;
    
    // ========== 委托 ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Flight")
    FOnFlightCompleted OnFlightCompleted;
    
    UPROPERTY(BlueprintAssignable, Category = "Flight")
    FOnCollisionDetected OnCollisionDetected;

    // ========== 重写基类导航 ==========
    // 统一接口：自动处理起飞，然后飞向目标
    virtual bool TryNavigateTo(FVector Destination) override;
    virtual void CancelNavigation() override;

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
    float ScanRadius = 300.f;

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

    // ========== Direct Control - Vertical Movement ==========
    // 垂直移动速度 (单位/秒)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|DirectControl")
    float VerticalMoveSpeed = 300.f;
    
    // 最小飞行高度 (相对于地面)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|DirectControl")
    float MinFlightAltitude = 100.f;
    
    // 最大飞行高度 (相对于地面)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|DirectControl")
    float MaxFlightAltitude = 2000.f;
    
    // 应用垂直移动输入 (Direction: 1.0 = 上升, -1.0 = 下降)
    UFUNCTION(BlueprintCallable, Category = "Control")
    void ApplyVerticalMovement(float Direction);

protected:
    virtual void SetupDroneAssets() PURE_VIRTUAL(AMADroneCharacter::SetupDroneAssets, );
    virtual void BeginPlay() override;
    
    // 能量系统
    void UpdateEnergyDisplay();
    void CheckLowEnergyStatus();
    
    // 动画
    void UpdatePropellerAnimation();
    
    UPROPERTY()
    UAnimSequence* PropellerAnim;
    
private:
    static constexpr float LowEnergyThreshold = 20.f;
    
    // 飞行系统内部
    void UpdateFlight(float DeltaTime);
    void SetFlightState(EMADroneFlightState NewState);
    float GetGroundHeight() const;
    
    // 起飞后待执行的飞行目标
    bool bHasPendingFlyTarget = false;
    FVector PendingFlyTarget;
};
