// MAMiniMapManager.cpp
// 小地图管理器实现

#include "MAMiniMapManager.h"
#include "MAMiniMapWidget.h"
#include "../HUD/MAHUD.h"
#include "../HUD/MAMainHUDWidget.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"

AMAMiniMapManager::AMAMiniMapManager()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建 Scene Capture 组件
    SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
    RootComponent = SceneCaptureComponent;
}

void AMAMiniMapManager::BeginPlay()
{
    Super::BeginPlay();

    SetupSceneCapture();
    CreateMiniMapWidget();
}

void AMAMiniMapManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Scene Capture 会自动更新，不需要手动处理
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

void AMAMiniMapManager::CreateMiniMapWidget()
{
    // NOTE: MiniMap Widget 现在由 MAMainHUDWidget 创建和管理
    // MAMiniMapManager 只负责 Scene Capture 和 RenderTarget
    // 如果需要初始化 MAMainHUDWidget 的 MiniMap，应该通过 MAHUD 获取 MainHUDWidget 并调用 InitializeMiniMap
    
    UE_LOG(LogTemp, Log, TEXT("[MiniMap] Scene Capture ready. Widget creation delegated to MAMainHUDWidget."));
    
    // 尝试初始化 MAMainHUDWidget 的 MiniMap
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC)
    {
        if (AHUD* HUD = PC->GetHUD())
        {
            // 尝试获取 MAHUD 并初始化其 MainHUDWidget 的 MiniMap
            if (AMAHUD* MAHUD = Cast<AMAHUD>(HUD))
            {
                UMAMainHUDWidget* MainHUDWidget = MAHUD->GetMainHUDWidget();
                if (MainHUDWidget)
                {
                    MainHUDWidget->InitializeMiniMap(RenderTarget, WorldBounds);
                    UE_LOG(LogTemp, Log, TEXT("[MiniMap] Initialized MAMainHUDWidget's MiniMap with RenderTarget"));
                }
            }
        }
    }
}

void AMAMiniMapManager::SetMiniMapVisible(bool bVisible)
{
    // MiniMap Widget 现在由 MAMainHUDWidget 管理
    // 这个方法保留用于向后兼容，但实际上不再需要
    UE_LOG(LogTemp, Log, TEXT("[MiniMap] SetMiniMapVisible called (no-op, widget managed by MAMainHUDWidget)"));
}

void AMAMiniMapManager::ToggleMiniMap()
{
    // MiniMap Widget 现在由 MAMainHUDWidget 管理
    // 这个方法保留用于向后兼容，但实际上不再需要
    UE_LOG(LogTemp, Log, TEXT("[MiniMap] ToggleMiniMap called (no-op, widget managed by MAMainHUDWidget)"));
}
