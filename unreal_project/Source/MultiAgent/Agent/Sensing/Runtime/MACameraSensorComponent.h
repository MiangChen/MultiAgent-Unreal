// MACameraSensorComponent.h
// 摄像头传感器组件 - 支持拍照、TCP流

#pragma once

#include "CoreMinimal.h"
#include "MASensorComponent.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "MACameraSensorComponent.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class FMASensingBootstrap;
struct FMASensingRuntimeBridge;

UCLASS(ClassGroup=(MultiAgent), meta=(BlueprintSpawnableComponent))
class MULTIAGENT_API UMACameraSensorComponent : public UMASensorComponent
{
    GENERATED_BODY()
    friend class FMASensingBootstrap;
    friend struct FMASensingRuntimeBridge;

public:
    UMACameraSensorComponent();
    virtual void OnRegister() override;
    virtual void BeginDestroy() override;

    // ========== 拍照功能 ==========
    UFUNCTION(BlueprintCallable, Category = "Camera")
    bool TakePhoto(const FString& FilePath = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Camera")
    TArray<FColor> CaptureFrame();

    // ========== 通用帧获取 ==========
    UFUNCTION(BlueprintCallable, Category = "Camera")
    TArray<uint8> GetFrameAsJPEG(int32 Quality = 85);

    // ========== TCP 流 ==========
    UFUNCTION(BlueprintCallable, Category = "Camera|Stream")
    bool StartTCPStream(int32 Port = 9000, float FPS = 30.f);

    UFUNCTION(BlueprintCallable, Category = "Camera|Stream")
    void StopTCPStream();

    UPROPERTY(BlueprintReadOnly, Category = "Camera|Stream")
    bool bIsStreaming = false;

    UPROPERTY(BlueprintReadOnly, Category = "Camera|Stream")
    int32 StreamPort = 9000;

    // ========== 属性 ==========
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FIntPoint Resolution = FIntPoint(640, 480);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float FOV = 90.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.5", ClampMax = "5.0"))
    float BrightnessMultiplier = 2.0f;

    // 流参数
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Stream", meta = (ClampMin = "10", ClampMax = "100"))
    int32 JPEGQuality = 50;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Stream", meta = (ClampMin = "5", ClampMax = "30"))
    float StreamFPS = 15.f;

    // ========== 获取子组件 ==========
    UFUNCTION(BlueprintCallable, Category = "Camera")
    UCameraComponent* GetCameraComponent() const { return CameraComponent; }

    UFUNCTION(BlueprintCallable, Category = "Camera")
    USceneCaptureComponent2D* GetSceneCaptureComponent() const { return SceneCaptureComponent; }

    UFUNCTION(BlueprintCallable, Category = "Camera")
    UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

    // ========== Action 动态发现与执行 ==========
    virtual TArray<FString> GetAvailableActions() const override;
    virtual bool ExecuteAction(const FString& ActionName, const TMap<FString, FString>& Params) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* CameraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USceneCaptureComponent2D* SceneCaptureComponent;

    UPROPERTY()
    UTextureRenderTarget2D* RenderTarget;

    void InitializeRenderTarget();

private:
    // TCP 流相关
    FTimerHandle StreamTimerHandle;
    FSocket* ListenSocket = nullptr;
    TArray<FSocket*> ClientSockets;
    void OnStreamTick();
    void AcceptNewClients();
    void SendFrameToClients(const TArray<uint8>& JPEGData);
    void CleanupSockets();
};
