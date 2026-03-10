// MAMiniMapManager.cpp
// 小地图管理器实现

#include "MAMiniMapManager.h"
#include "MAMiniMapWidget.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"

AMAMiniMapManager::AMAMiniMapManager()
{
    // 创建 Scene Capture 组件
    SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
    RootComponent = SceneCaptureComponent;
}

void AMAMiniMapManager::BeginPlay()
{
    Super::BeginPlay();

    SetupSceneCapture();
    InitializeMainHUDMiniMap();
}

void AMAMiniMapManager::SetupSceneCapture()
{
    if (!SceneCaptureComponent) return;

    // 创建 Render Target
    RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("MiniMapRenderTarget"));
    RenderTarget->InitAutoFormat(RenderTargetSize, RenderTargetSize);
    RenderTarget->UpdateResourceImmediate(true);

    // 配置 Scene Capture
    SceneCaptureComponent->TextureTarget = RenderTarget;
    SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
    SceneCaptureComponent->OrthoWidth = FMath::Max(WorldBounds.X, WorldBounds.Y);
    SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    
    // 设置捕获频率 (每帧更新以显示实时地图)
    SceneCaptureComponent->bCaptureEveryFrame = true;
    SceneCaptureComponent->bCaptureOnMovement = false;
    
    // 设置位置 (俯视)
    SetActorLocation(FVector(WorldCenter.X, WorldCenter.Y, CaptureHeight));
    SetActorRotation(FRotator(-90.f, 0.f, 0.f));  // 向下看

    // 隐藏一些不需要显示的内容
    SceneCaptureComponent->ShowFlags.SetFog(false);
    SceneCaptureComponent->ShowFlags.SetAtmosphere(false);
    SceneCaptureComponent->ShowFlags.SetBloom(false);
    SceneCaptureComponent->ShowFlags.SetMotionBlur(false);

    // 手动捕获一次
    SceneCaptureComponent->CaptureScene();

    UE_LOG(LogTemp, Log, TEXT("[MiniMap] Scene Capture initialized at height %.0f, ortho width %.0f"), 
        CaptureHeight, SceneCaptureComponent->OrthoWidth);
}

UMAMiniMapWidget* AMAMiniMapManager::ResolveMiniMapWidget()
{
    MiniMapWidget = HUDBridge.ResolveMiniMapWidget(this);
    return MiniMapWidget;
}

void AMAMiniMapManager::InitializeMainHUDMiniMap()
{
    UMAMiniMapWidget* ResolvedMiniMapWidget = ResolveMiniMapWidget();
    if (!ResolvedMiniMapWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MiniMap] MainHUDWidget or MiniMapWidget not ready"));
        return;
    }

    if (!RenderTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MiniMap] RenderTarget is null during initialization"));
        return;
    }

    ResolvedMiniMapWidget->InitializeMiniMap(RenderTarget, WorldBounds);
    UE_LOG(LogTemp, Log, TEXT("[MiniMap] Initialized MainHUDWidget MiniMap with RenderTarget"));
}

void AMAMiniMapManager::SetMiniMapVisible(bool bVisible)
{
    if (UMAMiniMapWidget* ResolvedMiniMapWidget = ResolveMiniMapWidget())
    {
        ResolvedMiniMapWidget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
        UE_LOG(LogTemp, Log, TEXT("[MiniMap] Visibility set to %s"), bVisible ? TEXT("Visible") : TEXT("Collapsed"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MiniMap] SetMiniMapVisible failed: MiniMapWidget not found"));
}

void AMAMiniMapManager::ToggleMiniMap()
{
    if (UMAMiniMapWidget* ResolvedMiniMapWidget = ResolveMiniMapWidget())
    {
        const bool bVisible = ResolvedMiniMapWidget->IsVisible();
        SetMiniMapVisible(!bVisible);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[MiniMap] ToggleMiniMap failed: MiniMapWidget not found"));
}
