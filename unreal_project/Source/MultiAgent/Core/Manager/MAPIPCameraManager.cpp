// MAPIPCameraManager.cpp
// 画中画相机管理器实现 - 使用 SceneCapture + HUD Canvas 绘制

#include "MAPIPCameraManager.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAPIPCamera, Log, All);

//=============================================================================
// UWorldSubsystem 接口
//=============================================================================

void UMAPIPCameraManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bInitialized = true;
    UE_LOG(LogMAPIPCamera, Log, TEXT("[PIPCameraManager] Initialized"));
}

void UMAPIPCameraManager::Deinitialize()
{
    DestroyAllPIPCameras();
    bInitialized = false;
    Super::Deinitialize();
    UE_LOG(LogMAPIPCamera, Log, TEXT("[PIPCameraManager] Deinitialized"));
}

//=============================================================================
// FTickableGameObject 接口
//=============================================================================

void UMAPIPCameraManager::Tick(float DeltaTime)
{
    UpdateFollowCameras();
}

//=============================================================================
// 公共 API
//=============================================================================

FGuid UMAPIPCameraManager::CreatePIPCamera(const FMAPIPCameraConfig& Config)
{
    FMAPIPCameraInstance Instance;
    Instance.CameraConfig = Config;
    
    // 创建渲染目标
    Instance.RenderTarget = CreateRenderTarget(Config.Resolution);
    if (!Instance.RenderTarget)
    {
        UE_LOG(LogMAPIPCamera, Error, TEXT("[PIPCameraManager] Failed to create RenderTarget"));
        return FGuid();
    }
    
    // 创建场景捕获
    Instance.SceneCapture = CreateSceneCapture(Config);
    if (!Instance.SceneCapture)
    {
        UE_LOG(LogMAPIPCamera, Error, TEXT("[PIPCameraManager] Failed to create SceneCapture"));
        return FGuid();
    }
    
    // 设置渲染目标
    Instance.SceneCapture->GetCaptureComponent2D()->TextureTarget = Instance.RenderTarget;
    
    FGuid CameraId = Instance.CameraId;
    CameraInstances.Add(CameraId, Instance);
    
    UE_LOG(LogMAPIPCamera, Log, TEXT("[PIPCameraManager] Created PIP camera: %s"), *CameraId.ToString());
    
    return CameraId;
}

bool UMAPIPCameraManager::ShowPIPCamera(const FGuid& CameraId, const FMAPIPDisplayConfig& DisplayConfig)
{
    FMAPIPCameraInstance* Instance = CameraInstances.Find(CameraId);
    if (!Instance || !Instance->IsValid())
    {
        UE_LOG(LogMAPIPCamera, Warning, TEXT("[PIPCameraManager] ShowPIPCamera: Camera not found or invalid: %s"), *CameraId.ToString());
        return false;
    }
    
    Instance->DisplayConfig = DisplayConfig;
    Instance->bIsVisible = true;
    
    // 启用场景捕获
    if (Instance->SceneCapture)
    {
        Instance->SceneCapture->GetCaptureComponent2D()->bCaptureEveryFrame = true;
    }
    
    UE_LOG(LogMAPIPCamera, Log, TEXT("[PIPCameraManager] Showing PIP camera: %s"), *CameraId.ToString());
    return true;
}

void UMAPIPCameraManager::HidePIPCamera(const FGuid& CameraId)
{
    FMAPIPCameraInstance* Instance = CameraInstances.Find(CameraId);
    if (!Instance || !Instance->bIsVisible)
    {
        return;
    }
    
    Instance->bIsVisible = false;
    
    if (Instance->SceneCapture)
    {
        Instance->SceneCapture->GetCaptureComponent2D()->bCaptureEveryFrame = false;
    }
    
    UE_LOG(LogMAPIPCamera, Log, TEXT("[PIPCameraManager] Hidden PIP camera: %s"), *CameraId.ToString());
}

bool UMAPIPCameraManager::UpdatePIPCamera(const FGuid& CameraId, const FMAPIPCameraConfig& Config)
{
    FMAPIPCameraInstance* Instance = CameraInstances.Find(CameraId);
    if (!Instance)
    {
        return false;
    }
    
    Instance->CameraConfig = Config;
    
    if (Instance->SceneCapture)
    {
        ApplyCameraConfig(Instance->SceneCapture, Config);
    }
    
    return true;
}

bool UMAPIPCameraManager::UpdatePIPDisplay(const FGuid& CameraId, const FMAPIPDisplayConfig& DisplayConfig)
{
    FMAPIPCameraInstance* Instance = CameraInstances.Find(CameraId);
    if (!Instance)
    {
        return false;
    }
    
    Instance->DisplayConfig = DisplayConfig;
    return true;
}

void UMAPIPCameraManager::DestroyPIPCamera(const FGuid& CameraId)
{
    FMAPIPCameraInstance* Instance = CameraInstances.Find(CameraId);
    if (!Instance)
    {
        return;
    }
    
    if (Instance->SceneCapture)
    {
        Instance->SceneCapture->Destroy();
    }
    
    CameraInstances.Remove(CameraId);
    UE_LOG(LogMAPIPCamera, Log, TEXT("[PIPCameraManager] Destroyed PIP camera: %s"), *CameraId.ToString());
}

void UMAPIPCameraManager::DestroyAllPIPCameras()
{
    TArray<FGuid> CameraIds;
    CameraInstances.GetKeys(CameraIds);
    
    for (const FGuid& CameraId : CameraIds)
    {
        DestroyPIPCamera(CameraId);
    }
}

bool UMAPIPCameraManager::DoesPIPCameraExist(const FGuid& CameraId) const
{
    return CameraInstances.Contains(CameraId);
}

bool UMAPIPCameraManager::IsPIPCameraVisible(const FGuid& CameraId) const
{
    const FMAPIPCameraInstance* Instance = CameraInstances.Find(CameraId);
    return Instance && Instance->bIsVisible;
}

int32 UMAPIPCameraManager::GetVisiblePIPCameraCount() const
{
    int32 Count = 0;
    for (const auto& Pair : CameraInstances)
    {
        if (Pair.Value.bIsVisible)
        {
            Count++;
        }
    }
    return Count;
}

void UMAPIPCameraManager::DrawPIPCameras(UCanvas* Canvas, int32 ViewportWidth, int32 ViewportHeight)
{
    if (!Canvas)
    {
        return;
    }

    for (const auto& Pair : CameraInstances)
    {
        const FMAPIPCameraInstance& Instance = Pair.Value;
        
        if (!Instance.bIsVisible || !Instance.RenderTarget)
        {
            continue;
        }

        const FMAPIPDisplayConfig& Config = Instance.DisplayConfig;

        // 计算位置和大小
        float PIPWidth = Config.Size.X;
        float PIPHeight = Config.Size.Y;
        float PIPX = Config.ScreenPosition.X * ViewportWidth - PIPWidth * 0.5f;
        float PIPY = Config.ScreenPosition.Y * ViewportHeight - PIPHeight * 0.5f;

        // 边界检查
        PIPX = FMath::Clamp(PIPX, 10.f, ViewportWidth - PIPWidth - 10.f);
        PIPY = FMath::Clamp(PIPY, 10.f, ViewportHeight - PIPHeight - 10.f);

        // 绘制多层渐变阴影 - 模拟柔和的投影效果
        if (Config.bShowShadow)
        {
            // 阴影参数
            const int32 ShadowLayers = 8;           // 阴影层数
            const float MaxShadowOffset = 16.f;     // 最大偏移
            const float MaxShadowExpand = 12.f;     // 最大扩展（让阴影比窗口大）
            const float BaseOpacity = 0.35f;        // 基础不透明度
            
            // 从外到内绘制阴影层，外层更淡更大，内层更深更小
            for (int32 i = ShadowLayers - 1; i >= 0; --i)
            {
                float LayerRatio = (float)i / (float)(ShadowLayers - 1);  // 0.0 (内层) -> 1.0 (外层)
                
                // 偏移量：内层偏移小，外层偏移大
                float LayerOffset = FMath::Lerp(4.f, MaxShadowOffset, LayerRatio);
                
                // 扩展量：内层扩展小，外层扩展大
                float LayerExpand = FMath::Lerp(0.f, MaxShadowExpand, LayerRatio);
                
                // 不透明度：内层深，外层淡（使用平方曲线让过渡更自然）
                float LayerOpacity = BaseOpacity * FMath::Pow(1.f - LayerRatio * 0.7f, 2.f);
                
                FCanvasTileItem ShadowItem(
                    FVector2D(PIPX + LayerOffset - LayerExpand * 0.5f, PIPY + LayerOffset - LayerExpand * 0.5f),
                    FVector2D(PIPWidth + LayerExpand, PIPHeight + LayerExpand),
                    FLinearColor(0.f, 0.f, 0.f, LayerOpacity)
                );
                ShadowItem.BlendMode = SE_BLEND_Translucent;
                Canvas->DrawItem(ShadowItem);
            }
        }

        // 绘制边框背景
        if (Config.bShowBorder)
        {
            float BorderThickness = Config.BorderThickness;
            FCanvasTileItem BorderItem(
                FVector2D(PIPX - BorderThickness, PIPY - BorderThickness),
                FVector2D(PIPWidth + BorderThickness * 2, PIPHeight + BorderThickness * 2),
                Config.BorderColor
            );
            BorderItem.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(BorderItem);
        }

        // 绘制 RenderTarget 纹理
        if (Instance.RenderTarget->GetResource())
        {
            FCanvasTileItem TileItem(
                FVector2D(PIPX, PIPY),
                Instance.RenderTarget->GetResource(),
                FVector2D(PIPWidth, PIPHeight),
                FLinearColor::White
            );
            TileItem.BlendMode = SE_BLEND_Opaque;
            Canvas->DrawItem(TileItem);
        }

        // 绘制标题栏
        if (!Config.Title.IsEmpty())
        {
            float TitleBarHeight = 24.f;
            
            // 标题栏背景
            FCanvasTileItem TitleBgItem(
                FVector2D(PIPX, PIPY),
                FVector2D(PIPWidth, TitleBarHeight),
                FLinearColor(0.f, 0.f, 0.f, 0.7f)
            );
            TitleBgItem.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(TitleBgItem);

            // 标题文字
            FCanvasTextItem TextItem(
                FVector2D(PIPX + 8.f, PIPY + 4.f),
                FText::FromString(Config.Title),
                GEngine->GetSmallFont(),
                FLinearColor::White
            );
            Canvas->DrawItem(TextItem);
        }
    }
}

//=============================================================================
// 内部方法
//=============================================================================

ASceneCapture2D* UMAPIPCameraManager::CreateSceneCapture(const FMAPIPCameraConfig& Config)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    ASceneCapture2D* SceneCapture = World->SpawnActor<ASceneCapture2D>(
        ASceneCapture2D::StaticClass(),
        Config.Location,
        Config.Rotation,
        SpawnParams
    );
    
    if (SceneCapture)
    {
        USceneCaptureComponent2D* CaptureComp = SceneCapture->GetCaptureComponent2D();
        if (CaptureComp)
        {
            CaptureComp->FOVAngle = Config.FOV;
            CaptureComp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
            CaptureComp->bCaptureEveryFrame = false;
            CaptureComp->bCaptureOnMovement = false;
            CaptureComp->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
            CaptureComp->bAlwaysPersistRenderingState = true;
        }
        
        SceneCapture->SetActorHiddenInGame(true);
    }
    
    return SceneCapture;
}

UTextureRenderTarget2D* UMAPIPCameraManager::CreateRenderTarget(const FIntPoint& Resolution)
{
    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this);
    if (RenderTarget)
    {
        RenderTarget->InitAutoFormat(Resolution.X, Resolution.Y);
        RenderTarget->UpdateResourceImmediate(true);
    }
    return RenderTarget;
}

void UMAPIPCameraManager::UpdateFollowCameras()
{
    for (auto& Pair : CameraInstances)
    {
        FMAPIPCameraInstance& Instance = Pair.Value;
        
        if (!Instance.CameraConfig.FollowTarget.IsValid())
        {
            continue;
        }
        
        AActor* Target = Instance.CameraConfig.FollowTarget.Get();
        if (!Target || !Instance.SceneCapture)
        {
            continue;
        }
        
        FVector NewLocation = Target->GetActorLocation() + Instance.CameraConfig.FollowOffset;
        FRotator NewRotation = Instance.CameraConfig.Rotation;
        
        if (Instance.CameraConfig.bLookAtTarget)
        {
            FVector DirectionToTarget = Target->GetActorLocation() - NewLocation;
            NewRotation = DirectionToTarget.Rotation();
        }
        
        Instance.SceneCapture->SetActorLocation(NewLocation);
        Instance.SceneCapture->SetActorRotation(NewRotation);
    }
}

void UMAPIPCameraManager::ApplyCameraConfig(ASceneCapture2D* SceneCapture, const FMAPIPCameraConfig& Config)
{
    if (!SceneCapture)
    {
        return;
    }
    
    SceneCapture->SetActorLocation(Config.Location);
    SceneCapture->SetActorRotation(Config.Rotation);
    
    if (USceneCaptureComponent2D* CaptureComp = SceneCapture->GetCaptureComponent2D())
    {
        CaptureComp->FOVAngle = Config.FOV;
    }
}
