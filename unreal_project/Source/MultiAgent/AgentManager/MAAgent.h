#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MAAgent.generated.h"

// Agent 类型枚举
UENUM(BlueprintType)
enum class EAgentType : uint8
{
    Human,
    RobotDog,
    Drone
};

UCLASS()
class MULTIAGENT_API AMAAgent : public ACharacter
{
    GENERATED_BODY()

public:
    AMAAgent();

    // 移动到指定位置
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveToLocation(FVector Destination);

    // 停止移动
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void StopMovement();

    // 获取当前位置
    UFUNCTION(BlueprintCallable, Category = "Agent")
    FVector GetCurrentLocation() const;

    // 更新动画（为动画蓝图提供加速度输入）
    void UpdateAnimation();

    // Agent ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    int32 AgentID;

    // Agent 名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    FString AgentName;

    // Agent 类型
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    EAgentType AgentType;

    // 是否正在移动
    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsMoving;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    FVector TargetLocation;
};
