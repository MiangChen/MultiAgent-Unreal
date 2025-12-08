// MAPlayerController.h
// 玩家控制器 - 使用 Enhanced Input System
// 支持星际争霸风格的框选和编组

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MAPlayerController.generated.h"

struct FInputActionValue;

// 鼠标左键模式
UENUM(BlueprintType)
enum class EMAMouseMode : uint8
{
    Select      UMETA(DisplayName = "Select"),      // 框选 Agent，禁用视角旋转
    Navigate    UMETA(DisplayName = "Navigate")     // Human 导航 + 视角旋转
};

UCLASS()
class MULTIAGENT_API AMAPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMAPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

    // ========== Input Handlers ==========
    void OnLeftClick(const FInputActionValue& Value);
    void OnLeftClickReleased(const FInputActionValue& Value);
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
    void OnTakePhoto(const FInputActionValue& Value);
    void OnToggleRecording(const FInputActionValue& Value);
    void OnToggleTCPStream(const FInputActionValue& Value);
    
    // ========== 编组快捷键 (Ctrl+0~9 / 0~9) ==========
    void OnControlGroup0(const FInputActionValue& Value);
    void OnControlGroup1(const FInputActionValue& Value);
    void OnControlGroup2(const FInputActionValue& Value);
    void OnControlGroup3(const FInputActionValue& Value);
    void OnControlGroup4(const FInputActionValue& Value);
    void OnControlGroup5(const FInputActionValue& Value);
    void OnControlGroup6(const FInputActionValue& Value);
    void OnControlGroup7(const FInputActionValue& Value);
    void OnControlGroup8(const FInputActionValue& Value);
    void OnControlGroup9(const FInputActionValue& Value);
    
    // 创建/解散 Squad 快捷键
    void OnCreateSquad(const FInputActionValue& Value);
    void OnDisbandSquad(const FInputActionValue& Value);

    // 切换鼠标模式
    void OnToggleMouseMode(const FInputActionValue& Value);

    // 获取鼠标点击位置
    bool GetMouseHitLocation(FVector& OutLocation);
    
    // 获取当前选中的相机
    class UMACameraSensorComponent* GetCurrentCamera();

    // ========== 框选 ==========
    void DrawSelectionBox();

private:
    // 当前查看的 Camera 索引（-1 表示上帝视角）
    int32 CurrentCameraIndex = -1;
    
    // 保存原始观察者 Pawn
    UPROPERTY()
    APawn* OriginalPawn = nullptr;
    

    
    // ========== 框选状态 ==========
    bool bIsBoxSelecting = false;
    FVector2D BoxSelectStart;
    FVector2D BoxSelectEnd;
    
public:
    // ========== 鼠标模式 ==========
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    EMAMouseMode CurrentMouseMode = EMAMouseMode::Select;
    
    // 获取模式名称
    UFUNCTION(BlueprintCallable, Category = "Input")
    static FString MouseModeToString(EMAMouseMode Mode);

private:
    
    // 处理编组快捷键
    void HandleControlGroup(int32 GroupIndex);
};
