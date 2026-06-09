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

    /**
     * 携带物体时物体相对 UAV 的附着偏移。
     * UAV 悬停在物体正上方的实现策略：物体悬挂在机体下方一段距离，
     * 因此偏移取一个负 Z 值。X/Y 为 0 让物体精准吊挂在质心下方。
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    FVector CarryAttachOffset = FVector(0.f, 0.f, -80.f);

    virtual FVector GetCarryAttachOffset() const override { return CarryAttachOffset; }

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
    void UpdatePropellerAnimation();

    UFUNCTION()
    void OnEnergyDepleted();
};
