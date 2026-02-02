// MAPIPCameraTypes.h
// 画中画相机系统类型定义

#pragma once

#include "CoreMinimal.h"
#include "MAPIPCameraTypes.generated.h"

class ASceneCapture2D;
class UTextureRenderTarget2D;

/**
 * 画中画相机配置
 * 定义相机的位置、朝向、视场角等参数
 */
USTRUCT(BlueprintType)
struct FMAPIPCameraConfig
{
    GENERATED_BODY()

    /** 相机位置 (世界坐标) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FVector Location = FVector::ZeroVector;

    /** 相机朝向 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FRotator Rotation = FRotator::ZeroRotator;

    /** 视场角 (度) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float FOV = 60.f;

    /** 渲染分辨率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FIntPoint Resolution = FIntPoint(512, 384);

    /** 可选：跟随目标 Actor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    TWeakObjectPtr<AActor> FollowTarget;

    /** 跟随偏移 (相对于目标) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FVector FollowOffset = FVector::ZeroVector;

    /** 是否朝向跟随目标 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    bool bLookAtTarget = false;

    FMAPIPCameraConfig() = default;

    FMAPIPCameraConfig(const FVector& InLocation, const FRotator& InRotation, float InFOV = 60.f)
        : Location(InLocation), Rotation(InRotation), FOV(InFOV)
    {
    }
};

/**
 * 画中画显示配置
 * 定义 Widget 在屏幕上的位置、大小、样式
 */
USTRUCT(BlueprintType)
struct FMAPIPDisplayConfig
{
    GENERATED_BODY()

    /** 屏幕位置 (归一化 0-1，相对于视口) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FVector2D ScreenPosition = FVector2D(0.65f, 0.55f);

    /** 显示大小 (像素) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FVector2D Size = FVector2D(400.f, 300.f);

    /** 可选标题 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FString Title;

    /** 是否显示边框 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    bool bShowBorder = true;

    /** 是否显示阴影 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    bool bShowShadow = true;

    /** 边框颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FLinearColor BorderColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.f);

    /** 边框厚度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    float BorderThickness = 2.f;

    /** 圆角半径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    float CornerRadius = 8.f;

    FMAPIPDisplayConfig() = default;
};

/**
 * 画中画相机实例数据
 * 内部使用，存储运行时数据
 */
USTRUCT()
struct FMAPIPCameraInstance
{
    GENERATED_BODY()

    /** 唯一标识符 */
    UPROPERTY()
    FGuid CameraId;

    /** 相机配置 */
    UPROPERTY()
    FMAPIPCameraConfig CameraConfig;

    /** 显示配置 */
    UPROPERTY()
    FMAPIPDisplayConfig DisplayConfig;

    /** 场景捕获 Actor */
    UPROPERTY()
    TObjectPtr<ASceneCapture2D> SceneCapture;

    /** 渲染目标纹理 */
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> RenderTarget;

    /** 是否正在显示 */
    UPROPERTY()
    bool bIsVisible = false;

    /** Widget 索引 (在 Container 中的位置) */
    UPROPERTY()
    int32 WidgetIndex = -1;

    FMAPIPCameraInstance()
    {
        CameraId = FGuid::NewGuid();
    }

    bool IsValid() const
    {
        return SceneCapture != nullptr && RenderTarget != nullptr;
    }
};
