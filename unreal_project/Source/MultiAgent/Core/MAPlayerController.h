// MAPlayerController.h
// 玩家控制器 - 使用 Enhanced Input System

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "MAPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UMAInputConfig;
struct FInputActionValue;

UCLASS()
class MULTIAGENT_API AMAPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMAPlayerController();

    // 输入映射上下文
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    // 输入配置
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UMAInputConfig> InputConfig;

    // ========== Input Actions ==========
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_LeftClick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_RightClick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Pickup;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Drop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_SpawnItem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_SpawnRobotDog;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_PrintInfo;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_DestroyLast;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_SwitchCamera;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ReturnSpectator;

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

    // 获取鼠标点击位置
    bool GetMouseHitLocation(FVector& OutLocation);

private:
    // 当前查看的 Camera 索引（-1 表示上帝视角）
    int32 CurrentCameraIndex = -1;
    
    // 保存原始观察者 Pawn
    UPROPERTY()
    APawn* OriginalPawn = nullptr;
};
