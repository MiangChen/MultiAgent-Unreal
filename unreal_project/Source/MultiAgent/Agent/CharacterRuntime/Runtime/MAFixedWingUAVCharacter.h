// MAFixedWingUAVCharacter.h
// 固定翼无人机 - 需要持续前进才能保持飞行
// 技能: Navigate, Search
//
// 飞行控制由 MANavigationService 中的 FlightController 统一管理

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAFixedWingUAVCharacter.generated.h"

class UMAStateTreeComponent;

UCLASS()
class MULTIAGENT_API AMAFixedWingUAVCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMAFixedWingUAVCharacter();

    // 飞行参数 (供 FlightController 参考)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float CruiseAltitude = 1500.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float MinSpeed = 300.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float MaxSpeed = 800.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float TurnRate = 45.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float OrbitRadius = 1000.f;
    
    // 避障参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleDetectionRange = 1200.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avoidance")
    float ObstacleAvoidanceRadius = 150.f;

    // StateTree
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UMAStateTreeComponent* StateTreeComponent;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void InitializeSkillSet() override;

private:
    UFUNCTION()
    void OnEnergyDepleted();
};
