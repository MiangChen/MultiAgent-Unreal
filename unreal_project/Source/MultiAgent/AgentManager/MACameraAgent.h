#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "MACameraAgent.generated.h"

/**
 * 摄像头 Agent - 上帝视角，可自由移动
 */
UCLASS()
class MULTIAGENT_API AMACameraAgent : public ASpectatorPawn
{
    GENERATED_BODY()

public:
    AMACameraAgent();

    // Agent ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    int32 AgentID;

    // Agent 名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
    FString AgentName;

    // 移动到指定位置
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveToLocation(FVector Destination);

    // 设置俯视角度
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void SetTopDownView(float Height = 1500.f, float Pitch = -60.f);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    FVector TargetLocation;
    bool bIsMoving;
    float MoveSpeed;
};
