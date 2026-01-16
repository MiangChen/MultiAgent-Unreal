// MAMiniMapManager.cpp
// 小地图管理器实现

#include "MAMiniMapManager.h"
#include "MAMiniMapWidget.h"
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
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;

    // 如果没有指定 Widget 类，使用默认的
    if (!MiniMapWidgetClass)
    {
        // 尝试加载默认 Widget Blueprint
        MiniMapWidgetClass = LoadClass<UMAMiniMapWidget>(
            nullptr, 
            TEXT("/Game/UI/WBP_MiniMap.WBP_MiniMap_C")
        );
    }

    // 如果还是没有，创建一个简单的 C++ Widget
    if (MiniMapWidgetClass)
    {
        MiniMapWidget = CreateWidget<UMAMiniMapWidget>(PC, MiniMapWidgetClass);
    }
    else
    {
        MiniMapWidget = CreateWidget<UMAMiniMapWidget>(PC, UMAMiniMapWidget::StaticClass());
    }

    if (MiniMapWidget)
    {
        MiniMapWidget->InitializeMiniMap(RenderTarget, WorldBounds);
        MiniMapWidget->WorldCenter = WorldCenter;
        MiniMapWidget->AddToViewport(10);  // 高优先级，显示在其他 UI 上面

        UE_LOG(LogTemp, Log, TEXT("[MiniMap] Widget created and added to viewport"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[MiniMap] Failed to create widget"));
    }
}

void AMAMiniMapManager::SetMiniMapVisible(bool bVisible)
{
    if (MiniMapWidget)
    {
        if (bVisible)
        {
            MiniMapWidget->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            MiniMapWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void AMAMiniMapManager::ToggleMiniMap()
{
    if (MiniMapWidget)
    {
        bool bCurrentlyVisible = MiniMapWidget->GetVisibility() == ESlateVisibility::Visible;
        SetMiniMapVisible(!bCurrentlyVisible);
    }
}
