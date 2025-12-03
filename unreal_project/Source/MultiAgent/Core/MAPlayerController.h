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

    // ===== 测试按键 =====
    // T - 生成一个机器狗
    void OnSpawnRobotDog();
    
    // Y - 打印当前 Agent 信息
    void OnPrintAgentInfo();
    
    // U - 销毁最后一个 Agent
    void OnDestroyLastAgent();

    // ===== GAS 测试按键 =====
    // P - 拾取物品
    void OnPickup();
    
    // O - 放下物品
    void OnDrop();
    
    // I - 生成可拾取方块
    void OnSpawnPickupItem();

    // ===== 视角切换 =====
    // Tab - 切换 Camera 视角
    void OnSwitchCamera();
    
    // 0 - 返回上帝视角
    void OnReturnToSpectator();

private:
    // 当前查看的 Camera 索引（-1 表示上帝视角）
    int32 CurrentCameraIndex = -1;
    
    // 保存原始观察者 Pawn
    UPROPERTY()
    APawn* OriginalPawn = nullptr;
};
