// MAMiniMapWidget.h
// 小地图 Widget - 显示俯视图、Agent 位置、相机指示器

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Application/MAMiniMapCoordinator.h"
#include "../Domain/MAMiniMapModels.h"
#include "MAMiniMapWidget.generated.h"

class UImage;
class UCanvasPanel;
class UTextureRenderTarget2D;
class UMAUITheme;

UCLASS()
class MULTIAGENT_API UMAMiniMapWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 初始化小地图 */
    UFUNCTION(BlueprintCallable, Category = "MiniMap")
    void InitializeMiniMap(UTextureRenderTarget2D* InRenderTarget, FVector2D InWorldBounds);

    /** 更新 Agent 位置 */
    UFUNCTION(BlueprintCallable, Category = "MiniMap")
    void UpdateAgentPositions();

    /** 更新相机位置 */
    UFUNCTION(BlueprintCallable, Category = "MiniMap")
    void UpdateCameraIndicator(FVector CameraLocation, FRotator CameraRotation);

    /** 应用主题 */
    UFUNCTION(BlueprintCallable, Category = "MiniMap|Theme")
    void ApplyTheme(UMAUITheme* InTheme);

    /** 获取当前主题 */
    UFUNCTION(BlueprintPure, Category = "MiniMap|Theme")
    UMAUITheme* GetTheme() const { return Theme; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

    /** 世界坐标转小地图坐标 */
    FVector2D WorldToMiniMap(FVector WorldLocation) const;

    /** 小地图坐标转世界坐标 */
    FVector MiniMapToWorld(FVector2D MiniMapLocation) const;

public:
    /** 小地图大小 (像素) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    float MiniMapSize = 200.f;

    /** 世界范围 (X, Y) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    FVector2D WorldBounds = FVector2D(20000.f, 20000.f);

    /** 世界中心 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    FVector2D WorldCenter = FVector2D(0.f, 0.f);

    /** Agent 图标大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    float AgentIconSize = 8.f;

    /** 相机图标大小 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    float CameraIconSize = 12.f;

    /** 相机视野指示器长度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    float CameraFOVLength = 30.f;

    /** 相机视野角度 (度) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
    float CameraFOVAngle = 60.f;

protected:
    /** 背景图片 (显示 RenderTarget) */
    UPROPERTY()
    UImage* BackgroundImage;

    /** Agent 图标容器 */
    UPROPERTY()
    UCanvasPanel* IconCanvas;

    /** 根 Canvas */
    UPROPERTY()
    UCanvasPanel* RootCanvas;

    /** 相机图标 (红色圆点) */
    UPROPERTY()
    UImage* CameraIcon;

    /** 相机视野左边线 */
    UPROPERTY()
    UImage* CameraFOVLeft;

    /** 相机视野右边线 */
    UPROPERTY()
    UImage* CameraFOVRight;

    /** 主题引用 */
    UPROPERTY()
    UMAUITheme* Theme;

private:
    /** Render Target 引用 */
    UPROPERTY()
    UTextureRenderTarget2D* RenderTarget;

    /** 更新计时器 */
    float UpdateTimer = 0.f;

    /** 更新间隔 (秒) */
    float UpdateInterval = 0.1f;

    /** 小地图协调器 */
    FMAMiniMapCoordinator Coordinator;

    /** 构建当前视口配置 */
    FMAMiniMapViewportConfig BuildViewportConfig() const;

    /** 应用当前帧模型 */
    void ApplyFrameModel(const FMAMiniMapFrameModel& Model);

    /** 应用相机指示器模型 */
    void ApplyCameraIndicator(const FMAMiniMapCameraIndicatorModel& Model);
};
