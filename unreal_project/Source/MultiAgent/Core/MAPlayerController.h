#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MAPlayerController.generated.h"

UCLASS()
class MULTIAGENT_API AMAPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMAPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;

    // 左键点击 - 移动玩家
    void OnLeftClick();
    
    // 右键点击 - 移动所有 Agent
    void OnRightClick();
    
    // 移动所有 Agent 到指定位置
    void MoveAllAgentsToLocation(FVector Destination);

    // 获取鼠标点击位置
    bool GetMouseHitLocation(FVector& OutLocation);
};
