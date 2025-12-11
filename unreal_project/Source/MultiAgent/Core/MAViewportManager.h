// MAViewportManager.h
// 视角管理器 - 管理相机视角切换
// 支持在不同 Agent 的相机之间切换，以及返回观察者视角
// 支持 Agent View Mode 下的 WASD 直接控制

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MAViewportManager.generated.h"

class AMACharacter;
class UMACameraSensorComponent;

UCLASS()
class MULTIAGENT_API UMAViewportManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // 切换到下一个相机
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    void SwitchToNextCamera(APlayerController* PC);

    // 切换到指定 Agent 的相机
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    void SwitchToAgentCamera(APlayerController* PC, AMACharacter* Agent);

    // 返回观察者视角
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    void ReturnToSpectator(APlayerController* PC);

    // 获取当前查看的相机
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    UMACameraSensorComponent* GetCurrentCamera() const;

    // 获取当前相机索引 (-1 表示观察者视角)
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    int32 GetCurrentCameraIndex() const { return CurrentCameraIndex; }

    // 获取所有可用相机
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    TArray<UMACameraSensorComponent*> GetAllCameras() const;

    // ========== Agent View Mode (Direct Control) ==========
    
    // 是否处于 Agent 视角模式
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    bool IsInAgentViewMode() const { return bIsInAgentViewMode; }
    
    // 获取当前控制的 Agent
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    AMACharacter* GetControlledAgent() const { return ControlledAgent.Get(); }
    
    // 应用移动输入 (WASD)
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    void ApplyMovementInput(FVector2D Input);
    
    // 应用垂直移动输入 (Space/Ctrl for Drone)
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    void ApplyVerticalInput(float Input);
    
    // 应用视角输入 (鼠标)
    UFUNCTION(BlueprintCallable, Category = "Viewport")
    void ApplyLookInput(FVector2D Input);

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    
    // 进入 Agent View Mode
    void EnterAgentViewMode(AMACharacter* Agent);
    
    // 退出 Agent View Mode
    void ExitAgentViewMode();

private:
    int32 CurrentCameraIndex = -1;
    
    UPROPERTY()
    TWeakObjectPtr<APawn> OriginalPawn;

    // 收集所有相机及其所属 Agent
    void CollectCameras(TArray<UMACameraSensorComponent*>& OutCameras, TArray<AMACharacter*>& OutOwners) const;
    
    // ========== Agent View Mode 状态 ==========
    
    // 是否处于 Agent 视角模式
    bool bIsInAgentViewMode = false;
    
    // 当前控制的 Agent
    UPROPERTY()
    TWeakObjectPtr<AMACharacter> ControlledAgent;
    
    // 相机俯仰角参数
    float DefaultCameraPitch = -15.f;
    float CurrentCameraPitch = -15.f;
    float MinCameraPitch = -60.f;
    float MaxCameraPitch = 30.f;
    
    // 鼠标视角灵敏度
    float LookSensitivityYaw = 14.0f;   // 水平旋转灵敏度
    float LookSensitivityPitch = 14.0f; // 垂直俯仰灵敏度
};
