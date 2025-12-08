// MAViewportManager.h
// 视角管理器 - 管理相机视角切换
// 支持在不同 Agent 的相机之间切换，以及返回观察者视角

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

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
    int32 CurrentCameraIndex = -1;
    
    UPROPERTY()
    TWeakObjectPtr<APawn> OriginalPawn;

    // 收集所有相机及其所属 Agent
    void CollectCameras(TArray<UMACameraSensorComponent*>& OutCameras, TArray<AMACharacter*>& OutOwners) const;
};
