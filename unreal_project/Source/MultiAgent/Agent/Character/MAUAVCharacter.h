// MAUAVCharacter.h
// 多旋翼无人机 (UAV) - 基于 DJI Inspire 2
// 技能: Navigate, Search, Follow
// 
// 飞行控制由 MANavigationService 中的 FlightController 统一管理

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAUAVCharacter.generated.h"

class UMAStateTreeComponent;

UCLASS()
class MULTIAGENT_API AMAUAVCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMAUAVCharacter();

    // 飞行参数 (从 ConfigManager 加载，供 FlightController 使用)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float DefaultFlightAltitude = 1000.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float MaxFlightSpeed = 600.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float AcceptanceRadius = 200.f;
    
    // 避障参数 (从 ConfigManager 加载)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleDetectionRange = 800.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleAvoidanceRadius = 150.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float MinFlightAltitude = 800.f;

    UFUNCTION(BlueprintCallable, Category = "Flight")
    bool IsInAir() const;

    // StateTree
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void InitializeSkillSet() override;
    
    UPROPERTY()
    UAnimSequence* PropellerAnim;

private:
    void SnapToGround();
    void UpdatePropellerAnimation();

    UFUNCTION()
    void OnEnergyDepleted();
};
