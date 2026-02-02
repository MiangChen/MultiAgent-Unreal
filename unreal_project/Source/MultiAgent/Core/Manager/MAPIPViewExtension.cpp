// MAPIPViewExtension.cpp
// 画中画渲染 - 使用 SceneCapture + Canvas 绘制方式
// 这是运行时最可靠的画中画实现方式

#include "MAPIPViewExtension.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAPIPViewExt, Log, All);

FMAPIPViewExtension::FMAPIPViewExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{
    UE_LOG(LogMAPIPViewExt, Log, TEXT("PIP ViewExtension created"));
}

FMAPIPViewExtension::~FMAPIPViewExtension()
{
    UE_LOG(LogMAPIPViewExt, Log, TEXT("PIP ViewExtension destroyed"));
}

bool FMAPIPViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
    FScopeLock Lock(&CriticalSection);
    return PIPCameras.Num() > 0;
}

void FMAPIPViewExtension::PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
    // 渲染线程中的绘制逻辑
    // 由于 Canvas 绘制需要在游戏线程，这里暂时留空
    // 实际绘制将通过 HUD 的 DrawHUD 完成
}

void FMAPIPViewExtension::DrawPIPCamera_RenderThread(FRHICommandListImmediate& RHICmdList, const FPIPCameraData& PIPData, const FIntRect& ViewportRect)
{
    // 预留给渲染线程绘制
}

void FMAPIPViewExtension::AddPIPCamera(const FGuid& CameraId, ACameraActor* Camera, UTextureRenderTarget2D* RenderTarget, const FMAPIPDisplayConfig& DisplayConfig)
{
    FScopeLock Lock(&CriticalSection);

    FPIPCameraData Data;
    Data.CameraId = CameraId;
    Data.Camera = Camera;
    Data.RenderTarget = RenderTarget;
    Data.DisplayConfig = DisplayConfig;

    PIPCameras.Add(CameraId, Data);

    UE_LOG(LogMAPIPViewExt, Log, TEXT("Added PIP camera: %s"), *CameraId.ToString());
}

void FMAPIPViewExtension::RemovePIPCamera(const FGuid& CameraId)
{
    FScopeLock Lock(&CriticalSection);
    PIPCameras.Remove(CameraId);
    UE_LOG(LogMAPIPViewExt, Log, TEXT("Removed PIP camera: %s"), *CameraId.ToString());
}

void FMAPIPViewExtension::UpdateDisplayConfig(const FGuid& CameraId, const FMAPIPDisplayConfig& DisplayConfig)
{
    FScopeLock Lock(&CriticalSection);
    
    if (FPIPCameraData* Data = PIPCameras.Find(CameraId))
    {
        Data->DisplayConfig = DisplayConfig;
    }
}

void FMAPIPViewExtension::ClearAllPIPCameras()
{
    FScopeLock Lock(&CriticalSection);
    PIPCameras.Empty();
    UE_LOG(LogMAPIPViewExt, Log, TEXT("Cleared all PIP cameras"));
}

