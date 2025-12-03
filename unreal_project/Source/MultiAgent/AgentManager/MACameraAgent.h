// MACameraAgent.h
// 摄像头 Agent - 支持拍照和录像

#pragma once

#include "CoreMinimal.h"
#include "MASensorAgent.h"
#include "MACameraAgent.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UCLASS()
class MULTIAGENT_API AMACameraAgent : public AMASensorAgent
{
    GENERATED_BODY()

public:
    AMACameraAgent();

    // ========== 拍照功能 ==========
    
    // 拍照并保存到文件
    UFUNCTION(BlueprintCallable, Category = "Camera")
    bool TakePhoto(const FString& FilePath = TEXT(""));

    // 获取当前帧图像数据
    UFUNCTION(BlueprintCallable, Category = "Camera")
    TArray<FColor> CaptureFrame();

    // ========== 属性 ==========
    
    // 图像分辨率
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FIntPoint Resolution = FIntPoint(1920, 1080);

    // 视场角
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float FOV = 90.0f;

protected:
    virtual void BeginPlay() override;

    // 摄像头组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* CameraComponent;

    // 场景捕获组件（用于截图）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USceneCaptureComponent2D* SceneCaptureComponent;

    // 渲染目标
    UPROPERTY()
    UTextureRenderTarget2D* RenderTarget;

    // 初始化渲染目标
    void InitializeRenderTarget();
};
