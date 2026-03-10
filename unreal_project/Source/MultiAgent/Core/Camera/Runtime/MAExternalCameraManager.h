// MAExternalCameraManager.h
// 外部摄像头管理器 - 管理固定摄像头和跟随摄像头
// 这些摄像头独立于 Agent 自身的相机传感器

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Core/Camera/Domain/MAExternalCameraTypes.h"
#include "MAExternalCameraManager.generated.h"

class AMACharacter;
class ACameraActor;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class MULTIAGENT_API UMAExternalCameraManager : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    
    // FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return !IsTemplate() && bInitialized; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMAExternalCameraManager, STATGROUP_Tickables); }
    virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
    virtual bool IsTickableWhenPaused() const override { return false; }
    virtual bool IsTickableInEditor() const override { return false; }
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

    // 初始化默认摄像头
    void InitializeDefaultCameras();

    // 添加外部摄像头
    UFUNCTION(BlueprintCallable, Category = "ExternalCamera")
    ACameraActor* AddExternalCamera(const FMAExternalCameraConfig& Config);

    // 获取所有外部摄像头
    UFUNCTION(BlueprintCallable, Category = "ExternalCamera")
    TArray<ACameraActor*> GetAllExternalCameras() const;

    // 获取外部摄像头数量
    UFUNCTION(BlueprintPure, Category = "ExternalCamera")
    int32 GetExternalCameraCount() const { return ExternalCameras.Num(); }

    // 获取指定索引的摄像头
    UFUNCTION(BlueprintCallable, Category = "ExternalCamera")
    ACameraActor* GetExternalCameraByIndex(int32 Index) const;

    // 获取摄像头名称
    UFUNCTION(BlueprintCallable, Category = "ExternalCamera")
    FString GetCameraName(int32 Index) const;

    // 获取指定索引的视角目标 Actor (用于 SetViewTarget)
    // 对于弹簧臂相机返回 SpringArmCameraActor，否则返回 CameraActor
    UFUNCTION(BlueprintCallable, Category = "ExternalCamera")
    AActor* GetViewTargetByIndex(int32 Index) const;

private:
    // 外部摄像头列表
    UPROPERTY()
    TArray<FMAExternalCameraInstance> ExternalCameras;

    // 是否已初始化
    bool bInitialized = false;

    // 创建摄像头 Actor
    ACameraActor* CreateCameraActor(const FMAExternalCameraConfig& Config);

    // 创建带弹簧臂的摄像头 Actor
    AActor* CreateSpringArmCameraActor(const FMAExternalCameraConfig& Config, USpringArmComponent*& OutSpringArm, UCameraComponent*& OutCamera);

    // 更新跟随摄像头位置
    void UpdateFollowCameras();

    // 查找目标 Agent
    AMACharacter* FindAgentByLabel(const FString& Label) const;
};
