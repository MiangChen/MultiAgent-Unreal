// MAPlayerController.h
// 玩家控制器 - 使用 Enhanced Input System

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MAPlayerController.generated.h"

struct FInputActionValue;

UCLASS()
class MULTIAGENT_API AMAPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMAPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    // ========== Input Handlers ==========
    void OnLeftClick(const FInputActionValue& Value);
    void OnRightClick(const FInputActionValue& Value);
    void OnPickup(const FInputActionValue& Value);
    void OnDrop(const FInputActionValue& Value);
    void OnSpawnPickupItem(const FInputActionValue& Value);
    void OnSpawnRobotDog(const FInputActionValue& Value);
    void OnPrintAgentInfo(const FInputActionValue& Value);
    void OnDestroyLastAgent(const FInputActionValue& Value);
    void OnSwitchCamera(const FInputActionValue& Value);
    void OnReturnToSpectator(const FInputActionValue& Value);
    void OnStartPatrol(const FInputActionValue& Value);
    void OnStartCharge(const FInputActionValue& Value);
    void OnStopIdle(const FInputActionValue& Value);
    void OnStartCoverage(const FInputActionValue& Value);
    void OnStartFollow(const FInputActionValue& Value);
    void OnStartAvoid(const FInputActionValue& Value);
    void OnStartFormation(const FInputActionValue& Value);

    // 获取鼠标点击位置
    bool GetMouseHitLocation(FVector& OutLocation);

private:
    // 当前查看的 Camera 索引（-1 表示上帝视角）
    int32 CurrentCameraIndex = -1;
    
    // 保存原始观察者 Pawn
    UPROPERTY()
    APawn* OriginalPawn = nullptr;
    
    // 当前编队类型索引 (0=None/Stop, 1=Line, 2=Column, 3=Wedge, 4=Diamond, 5=Circle)
    int32 CurrentFormationIndex = 0;
};
