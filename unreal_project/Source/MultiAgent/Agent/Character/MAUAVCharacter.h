// MAUAVCharacter.h
// 多旋翼无人机 (UAV) - 基于 DJI Inspire 2
// 技能: Navigate, Search, Follow

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAUAVCharacter.generated.h"

class UMAStateTreeComponent;

UENUM(BlueprintType)
enum class EMAFlightState : uint8
{
    Landed,
    TakingOff,
    Hovering,
    Flying,
    Landing
};

UCLASS()
class MULTIAGENT_API AMAUAVCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMAUAVCharacter();
    virtual void Tick(float DeltaTime) override;

    // 飞行参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float DefaultFlightAltitude = 1000.f;  // 10m
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float MaxFlightSpeed = 400.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float AcceptanceRadius = 100.f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    EMAFlightState FlightState = EMAFlightState::Landed;
    
    // 避障参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleDetectionRange = 500.f;  // 障碍物检测距离
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleAvoidanceRadius = 100.f;  // 避障检测半径

    // 飞行控制
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool TakeOff(float TargetAltitude = -1.f);
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool Land();
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void Hover();
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool FlyTo(FVector Destination);

    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool IsInAir() const { return FlightState != EMAFlightState::Landed; }
    
    /** 设置飞行状态 */
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void SetFlightState(EMAFlightState NewState);

    // 重写导航
    virtual bool TryNavigateTo(FVector Destination) override;
    virtual void CancelNavigation() override;

    // StateTree
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

protected:
    virtual void BeginPlay() override;
    virtual void InitializeSkillSet() override;
    
    UPROPERTY()
    UAnimSequence* PropellerAnim;

private:
    FVector CurrentFlightTarget;
    float CurrentSpeed = 0.f;
    bool bHasPendingFlyTarget = false;
    FVector PendingFlyTarget;

    void UpdateFlight(float DeltaTime);
    float GetGroundHeight() const;
    void SnapToGround();
    
    /** 计算避障方向 */
    FVector CalculateAvoidanceDirection(const FVector& DesiredDirection);

    UFUNCTION()
    void OnEnergyDepleted();
};
