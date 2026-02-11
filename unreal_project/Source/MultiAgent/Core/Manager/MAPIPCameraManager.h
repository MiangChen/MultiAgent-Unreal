// MAPIPCameraManager.h
// 画中画相机管理器 - 管理多个画中画相机实例
// 提供创建、显示、隐藏、更新、销毁 API

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "../Types/MAPIPCameraTypes.h"
#include "MAPIPCameraManager.generated.h"

class UCanvas;

/**
 * 画中画相机管理器
 * 
 * 功能：
 * - 创建/销毁画中画相机实例
 * - 显示/隐藏画中画窗口
 * - 更新相机参数（位置、朝向、FOV）
 * - 支持跟随目标 Actor
 * - 管理多个同时显示的画中画
 * 
 * 使用示例：
 * ```cpp
 * // 获取管理器
 * UMAPIPCameraManager* PIPManager = GetWorld()->GetSubsystem<UMAPIPCameraManager>();
 * 
 * // 创建相机
 * FMAPIPCameraConfig CameraConfig;
 * CameraConfig.Location = FVector(0, 0, 500);
 * CameraConfig.Rotation = FRotator(-45, 0, 0);
 * CameraConfig.FOV = 60.f;
 * FGuid CameraId = PIPManager->CreatePIPCamera(CameraConfig);
 * 
 * // 显示
 * FMAPIPDisplayConfig DisplayConfig;
 * DisplayConfig.ScreenPosition = FVector2D(0.7f, 0.6f);
 * DisplayConfig.Size = FVector2D(400, 300);
 * DisplayConfig.Title = TEXT("Camera View");
 * PIPManager->ShowPIPCamera(CameraId, DisplayConfig);
 * 
 * // 隐藏
 * PIPManager->HidePIPCamera(CameraId);
 * 
 * // 销毁
 * PIPManager->DestroyPIPCamera(CameraId);
 * ```
 */
UCLASS()
class MULTIAGENT_API UMAPIPCameraManager : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    //=========================================================================
    // UWorldSubsystem 接口
    //=========================================================================
    
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    //=========================================================================
    // FTickableGameObject 接口
    //=========================================================================
    
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return !IsTemplate() && bInitialized; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMAPIPCameraManager, STATGROUP_Tickables); }
    virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
    virtual bool IsTickableWhenPaused() const override { return false; }
    virtual bool IsTickableInEditor() const override { return false; }
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

    //=========================================================================
    // 公共 API
    //=========================================================================

    /**
     * 创建画中画相机
     * @param Config 相机配置
     * @return 相机唯一标识符
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    FGuid CreatePIPCamera(const FMAPIPCameraConfig& Config);

    /**
     * 显示画中画相机
     * @param CameraId 相机标识符
     * @param DisplayConfig 显示配置
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    bool ShowPIPCamera(const FGuid& CameraId, const FMAPIPDisplayConfig& DisplayConfig);

    /**
     * 隐藏画中画相机
     * @param CameraId 相机标识符
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void HidePIPCamera(const FGuid& CameraId);

    /**
     * 更新相机配置
     * @param CameraId 相机标识符
     * @param Config 新的相机配置
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    bool UpdatePIPCamera(const FGuid& CameraId, const FMAPIPCameraConfig& Config);

    /**
     * 更新显示配置
     * @param CameraId 相机标识符
     * @param DisplayConfig 新的显示配置
     * @return 是否成功
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    bool UpdatePIPDisplay(const FGuid& CameraId, const FMAPIPDisplayConfig& DisplayConfig);

    /**
     * 销毁画中画相机
     * @param CameraId 相机标识符
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void DestroyPIPCamera(const FGuid& CameraId);

    /**
     * 销毁所有画中画相机
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    void DestroyAllPIPCameras();

    /**
     * 检查相机是否存在
     * @param CameraId 相机标识符
     * @return 是否存在
     */
    UFUNCTION(BlueprintPure, Category = "PIPCamera")
    bool DoesPIPCameraExist(const FGuid& CameraId) const;

    /**
     * 检查相机是否正在显示
     * @param CameraId 相机标识符
     * @return 是否正在显示
     */
    UFUNCTION(BlueprintPure, Category = "PIPCamera")
    bool IsPIPCameraVisible(const FGuid& CameraId) const;

    /**
     * 获取当前显示的画中画数量
     * @return 显示数量
     */
    UFUNCTION(BlueprintPure, Category = "PIPCamera")
    int32 GetVisiblePIPCameraCount() const;

    /**
     * 分配不重叠的屏幕位置
     * @param Size 画中画尺寸
     * @return 分配的屏幕位置 (归一化 0-1)
     */
    UFUNCTION(BlueprintCallable, Category = "PIPCamera")
    FVector2D AllocateScreenPosition(const FVector2D& Size) const;

    /**
     * 绘制所有画中画相机 (由 HUD::DrawHUD 调用)
     * @param Canvas 画布
     * @param ViewportWidth 视口宽度
     * @param ViewportHeight 视口高度
     */
    void DrawPIPCameras(UCanvas* Canvas, int32 ViewportWidth, int32 ViewportHeight);

private:
    //=========================================================================
    // 内部数据
    //=========================================================================

    /** 所有相机实例 */
    UPROPERTY()
    TMap<FGuid, FMAPIPCameraInstance> CameraInstances;

    /** 是否已初始化 */
    bool bInitialized = false;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 创建场景捕获 Actor */
    ASceneCapture2D* CreateSceneCapture(const FMAPIPCameraConfig& Config);

    /** 创建渲染目标纹理 */
    UTextureRenderTarget2D* CreateRenderTarget(const FIntPoint& Resolution);

    /** 更新跟随相机位置 */
    void UpdateFollowCameras();

    /** 应用相机配置到场景捕获 */
    void ApplyCameraConfig(ASceneCapture2D* SceneCapture, const FMAPIPCameraConfig& Config);
};
