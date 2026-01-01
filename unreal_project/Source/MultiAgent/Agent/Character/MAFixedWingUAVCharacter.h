// MAFixedWingUAVCharacter.h
// 固定翼无人机 - 需要持续前进才能保持飞行
// 技能: Navigate, Search

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAFixedWingUAVCharacter.generated.h"

class UMAStateTreeComponent;

UENUM(BlueprintType)
enum class EMAFixedWingFlightMode : uint8
{
    Cruising,       // 巡航（直线飞行）
    Navigating,     // 导航到目标
    Orbiting        // 盘旋（绕圈）
};

UCLASS()
class MULTIAGENT_API AMAFixedWingUAVCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMAFixedWingUAVCharacter();
    virtual void Tick(float DeltaTime) override;

    // 飞行参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float CruiseAltitude = 800.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float MinSpeed = 200.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float MaxSpeed = 600.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float TurnRate = 45.f;  // 度/秒
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float OrbitRadius = 800.f;  // 盘旋半径
    
    // 避障参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleDetectionRange = 800.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleAvoidanceRadius = 150.f;

    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    bool bIsFlying = false;
    
    UPROPERTY(BlueprintReadOnly, Category = "Flight")
    EMAFixedWingFlightMode FlightMode = EMAFixedWingFlightMode::Cruising;

    // 飞行控制
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool StartFlight();
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void StopFlight();
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool FlyTowards(FVector Destination);
    
    UFUNCTION(BlueprintCallable, Category = "Flight")
    void StartOrbit(FVector OrbitCenter);

    // 重写导航
    virtual bool TryNavigateTo(FVector Destination) override;
    virtual void CancelNavigation() override;

    // StateTree
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

protected:
    virtual void BeginPlay() override;
    virtual void InitializeSkillSet() override;

private:
    FVector TargetLocation;
    FVector OrbitCenter;
    float TargetAltitude = 800.f;
    float CurrentSpeed = 0.f;
    float OrbitAngle = 0.f;  // 当前盘旋角度

    void UpdateFlight(float DeltaTime);
    void UpdateNavigation(float DeltaTime);
    void UpdateOrbit(float DeltaTime);
    FVector CalculateAvoidanceDirection(const FVector& DesiredDirection);

    UFUNCTION()
    void OnEnergyDepleted();
};
