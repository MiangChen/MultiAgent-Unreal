// MADroneCharacter.h
// 无人机角色基类 - 支持 3D 飞行、悬停、StateTree AI
// 子类: MADronePhantom4Character, MADroneInspire2Character
// 使用 Capability Component 模式管理能力参数
//
// 飞行系统说明:
// - 不使用 NavMesh，采用直接位置控制
// - 支持碰撞检测，便于未来开发避障算法
// - 状态机: Landed → TakeOff → Hovering ↔ Flying → Land → Landed

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MADroneCharacter.generated.h"

class UMAStateTreeComponent;
class UMAEnergyComponent;
class UMAPatrolComponent;
class UMAFollowComponent;
class UMACoverageComponent;

// 无人机飞行状态
UENUM(BlueprintType)
enum class EMADroneFlightState : uint8
{
    Landed      UMETA(DisplayName = "Landed"),
    TakingOff   UMETA(DisplayName = "TakingOff"),
    Hovering    UMETA(DisplayName = "Hovering"),
    Flying      UMETA(DisplayName = "Flying"),
    Landing     UMETA(DisplayName = "Landing")
};

// 飞行完成委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlightCompleted, bool, bSuccess, FVector, FinalLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCollisionDetected, FHitResult, HitResult);

UCLASS(Abstract)
class MULTIAGENT_API AMADroneCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMADroneCharacter();

    virtual void Tick(float DeltaTime) override;

    // ========== Capability Components ==========
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    UMAEnergyComponent* EnergyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    UMAPatrolComponent* PatrolComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    UMAFollowComponent* FollowComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capability")
    UMACoverageComponent* CoverageComponent;

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
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool TakeOff(float TargetAltitude = -1.f);
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool Land();
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void Hover();
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool FlyTo(FVector Destination);
    
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
    
    UFUNCTION(BlueprintCallable, Category = "Flight|Collision")
    bool CheckForwardCollision(FHitResult& OutHit) const;
    
    UFUNCTION(BlueprintCallable, Category = "Flight|Collision")
    bool CheckCollisionInDirection(FVector Direction, float Distance, FHitResult& OutHit) const;
    
    // ========== 委托 ==========
    
    UPROPERTY(BlueprintAssignable, Category = "Flight")
    FOnFlightCompleted OnFlightCompleted;
    
    UPROPERTY(BlueprintAssignable, Category = "Flight")
    FOnCollisionDetected OnCollisionDetected;

    // ========== 重写基类导航 ==========
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

    // ========== Direct Control - Vertical Movement ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|DirectControl")
    float VerticalMoveSpeed = 300.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|DirectControl")
    float MinFlightAltitude = 100.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|DirectControl")
    float MaxFlightAltitude = 2000.f;
    
    UFUNCTION(BlueprintCallable, Category = "Control")
    void ApplyVerticalMovement(float Direction);

    // ========== 便捷访问方法 ==========
    UFUNCTION(BlueprintCallable, Category = "Energy")
    float GetEnergy() const;

    UFUNCTION(BlueprintCallable, Category = "Energy")
    bool HasEnergy() const;

protected:
    virtual void SetupDroneAssets() PURE_VIRTUAL(AMADroneCharacter::SetupDroneAssets, );
    virtual void BeginPlay() override;
    
    // 动画
    void UpdatePropellerAnimation();
    
    UPROPERTY()
    UAnimSequence* PropellerAnim;
    
private:
    // 飞行系统内部
    void UpdateFlight(float DeltaTime);
    void SetFlightState(EMADroneFlightState NewState);
    float GetGroundHeight() const;
    
    // 起飞后待执行的飞行目标
    bool bHasPendingFlyTarget = false;
    FVector PendingFlyTarget;

    // 能量耗尽回调
    UFUNCTION()
    void OnEnergyDepleted();
};
