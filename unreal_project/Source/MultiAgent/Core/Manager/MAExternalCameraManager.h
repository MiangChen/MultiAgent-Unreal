// MAExternalCameraManager.h
// 外部摄像头管理器 - 管理固定摄像头和跟随摄像头
// 这些摄像头独立于 Agent 自身的相机传感器

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "GameFramework/Actor.h"
#include "MAExternalCameraManager.generated.h"

class AMACharacter;
class ACameraActor;
class USpringArmComponent;
class UCameraComponent;

// 外部摄像头类型
UENUM(BlueprintType)
enum class EMAExternalCameraType : uint8
{
    Fixed      UMETA(DisplayName = "Fixed"),      // 固定位置摄像头
    Follow     UMETA(DisplayName = "Follow")      // 跟随 Agent 摄像头
};

// 跟随摄像头视角类型
UENUM(BlueprintType)
enum class EMAFollowViewType : uint8
{
    Behind          UMETA(DisplayName = "Behind"),          // 身后视角
    TopDown         UMETA(DisplayName = "TopDown"),         // 头顶俯视
    BehindElevated  UMETA(DisplayName = "BehindElevated"),  // 身后抬高俯视
    BehindElevatedReverse UMETA(DisplayName = "BehindElevatedReverse") // 身后抬高俯视（反向，真正的身后）
};

// 外部摄像头配置
USTRUCT(BlueprintType)
struct FMAExternalCameraConfig
{
    GENERATED_BODY()

    // 摄像头名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CameraName;

    // 摄像头类型
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMAExternalCameraType CameraType = EMAExternalCameraType::Fixed;

    // 固定摄像头位置 (仅 Fixed 类型)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;

    // 固定摄像头朝向 (仅 Fixed 类型)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;

    // 跟随目标 Agent 标签 (仅 Follow 类型)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString TargetAgentLabel;

    // 跟随视角类型 (仅 Follow 类型)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMAFollowViewType FollowViewType = EMAFollowViewType::Behind;

    // 跟随距离
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FollowDistance = 500.f;

    // 跟随高度 (用于 BehindElevated 类型)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FollowHeight = 100.f;

    // 是否使用弹簧臂 (启用相机延迟)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseSpringArm = false;

    // 相机位置延迟速度 (仅弹簧臂模式)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CameraLagSpeed = 10.f;

    // 相机旋转延迟速度 (仅弹簧臂模式)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CameraRotationLagSpeed = 10.f;

    // 视野角度
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FOV = 90.f;
};

// 外部摄像头实例
USTRUCT()
struct FMAExternalCameraInstance
{
    GENERATED_BODY()

    UPROPERTY()
    FMAExternalCameraConfig Config;

    UPROPERTY()
    ACameraActor* CameraActor = nullptr;

    // 弹簧臂相机 Actor (用于带延迟的跟随相机)
    UPROPERTY()
    AActor* SpringArmCameraActor = nullptr;

    // 弹簧臂组件引用
    UPROPERTY()
    USpringArmComponent* SpringArmComponent = nullptr;

    // 相机组件引用
    UPROPERTY()
    UCameraComponent* CameraComponent = nullptr;

    UPROPERTY()
    TWeakObjectPtr<AMACharacter> TargetAgent;
};

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
