#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MACharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class MULTIAGENT_API AMACharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMACharacter();

    // 移动到指定位置
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveToLocation(FVector Destination);

    // 停止移动
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StopMovement();

    // 获取当前位置
    UFUNCTION(BlueprintCallable, Category = "Movement")
    FVector GetCurrentLocation() const;

    // Agent ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    int32 AgentID;

    // Agent 名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    FString AgentName;

    // 摄像机臂
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* CameraBoom;

    // 俯视摄像机
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* TopDownCamera;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    FVector TargetLocation;
    bool bIsMoving;
};
