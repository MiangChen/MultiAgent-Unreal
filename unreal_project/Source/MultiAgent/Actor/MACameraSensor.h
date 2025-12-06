// MACameraSensor.h
// 摄像头传感器 - 支持拍照

#pragma once

#include "CoreMinimal.h"
#include "MASensor.h"
#include "MACameraSensor.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UCLASS()
class MULTIAGENT_API AMACameraSensor : public AMASensor
{
    GENERATED_BODY()

public:
    AMACameraSensor();

    // ========== 拍照功能 ==========
    UFUNCTION(BlueprintCallable, Category = "Camera")
    bool TakePhoto(const FString& FilePath = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Camera")
    TArray<FColor> CaptureFrame();

    // ========== 属性 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FIntPoint Resolution = FIntPoint(1920, 1080);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float FOV = 90.0f;
    
    // 亮度调整倍数（用于匹配编辑器视图）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.5", ClampMax = "5.0"))
    float BrightnessMultiplier = 2.0f;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* CameraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USceneCaptureComponent2D* SceneCaptureComponent;

    UPROPERTY()
    UTextureRenderTarget2D* RenderTarget;

    void InitializeRenderTarget();
};
