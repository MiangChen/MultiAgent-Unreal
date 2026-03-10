// MAPIPViewExtension.h
// 画中画 SceneViewExtension - 在渲染管线中直接叠加绘制相机视图
// 类似编辑器的 Camera Preview 效果

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "Core/Camera/Domain/MAPIPCameraTypes.h"

class ACameraActor;
class UTextureRenderTarget2D;

/**
 * 画中画视图扩展
 * 
 * 使用 FSceneViewExtension 在渲染管线的 PostRenderViewFamily 阶段
 * 直接将相机视图叠加绘制到主 Viewport 上
 */
class MULTIAGENT_API FMAPIPViewExtension : public FSceneViewExtensionBase
{
public:
    FMAPIPViewExtension(const FAutoRegister& AutoRegister);
    virtual ~FMAPIPViewExtension();

    //=========================================================================
    // FSceneViewExtensionBase 接口
    //=========================================================================
    
    virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
    
    virtual void PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}
    virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override {}
    
    // 在这里绘制画中画
    virtual void PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override;
    
    virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

    //=========================================================================
    // 画中画管理
    //=========================================================================

    /** 添加画中画相机 */
    void AddPIPCamera(const FGuid& CameraId, ACameraActor* Camera, UTextureRenderTarget2D* RenderTarget, const FMAPIPDisplayConfig& DisplayConfig);

    /** 移除画中画相机 */
    void RemovePIPCamera(const FGuid& CameraId);

    /** 更新显示配置 */
    void UpdateDisplayConfig(const FGuid& CameraId, const FMAPIPDisplayConfig& DisplayConfig);

    /** 清除所有画中画 */
    void ClearAllPIPCameras();

    /** 是否有活动的画中画 */
    bool HasActivePIPCameras() const { return PIPCameras.Num() > 0; }

private:
    /** 画中画相机数据 */
    struct FPIPCameraData
    {
        FGuid CameraId;
        TWeakObjectPtr<ACameraActor> Camera;
        TWeakObjectPtr<UTextureRenderTarget2D> RenderTarget;
        FMAPIPDisplayConfig DisplayConfig;
    };

    /** 所有画中画相机 */
    TMap<FGuid, FPIPCameraData> PIPCameras;

    /** 线程安全锁 */
    mutable FCriticalSection CriticalSection;

    /** 绘制单个画中画 */
    void DrawPIPCamera_RenderThread(FRHICommandListImmediate& RHICmdList, const FPIPCameraData& PIPData, const FIntRect& ViewportRect);
};
