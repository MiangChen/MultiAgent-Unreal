// MAMiniMapManager.h
// 小地图管理器 - 管理 Scene Capture 和 Widget

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Infrastructure/MAMiniMapRuntimeAdapter.h"
#include "MAMiniMapManager.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMAMiniMapWidget;

UCLASS()
class MULTIAGENT_API AMAMiniMapManager : public AActor
{
    GENERATED_BODY()

public:
    AMAMiniMapManager();

protected:
    virtual void BeginPlay() override;

public:
    /** 获取 Render Target */
    UFUNCTION(BlueprintPure, Category = "MiniMap")
    UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

    /** 获取世界范围 */
    UFUNCTION(BlueprintPure, Category = "MiniMap")
    FVector2D GetWorldBounds() const { return WorldBounds; }

    /** 显示/隐藏小地图 */
    UFUNCTION(BlueprintCallable, Category = "MiniMap")
    void SetMiniMapVisible(bool bVisible);

    /** 切换小地图显示 */
    UFUNCTION(BlueprintCallable, Category = "MiniMap")
    void ToggleMiniMap();

public:
    /** 俯视相机高度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    float CaptureHeight = 8000.f;

    /** 世界范围 (覆盖的 XY 范围) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    FVector2D WorldBounds = FVector2D(20000.f, 20000.f);

    /** 世界中心 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    FVector2D WorldCenter = FVector2D(0.f, 0.f);

    /** Render Target 分辨率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    int32 RenderTargetSize = 512;

protected:
    /** Scene Capture 组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MiniMap")
    USceneCaptureComponent2D* SceneCaptureComponent;

    /** Render Target */
    UPROPERTY()
    UTextureRenderTarget2D* RenderTarget;

    /** Widget 实例 */
    UPROPERTY()
    UMAMiniMapWidget* MiniMapWidget;

private:
    /** 初始化 Scene Capture */
    void SetupSceneCapture();

    /** 解析并缓存当前主 HUD 上的小地图组件 */
    UMAMiniMapWidget* ResolveMiniMapWidget();

    /** 初始化主 HUD 上的小地图组件 */
    void InitializeMainHUDMiniMap();

    /** HUD 解析桥 */
    FMAMiniMapManagerHUDBridge HUDBridge;
};
