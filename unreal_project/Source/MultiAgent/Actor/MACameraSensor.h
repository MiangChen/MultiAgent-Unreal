// MACameraSensor.h
// 摄像头传感器 - 支持拍照、录像、TCP流

#pragma once

#include "CoreMinimal.h"
#include "MASensor.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
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
    virtual void BeginDestroy() override;

    // ========== 拍照功能 ==========
    UFUNCTION(BlueprintCallable, Category = "Camera")
    bool TakePhoto(const FString& FilePath = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "Camera")
    TArray<FColor> CaptureFrame();

    // ========== 通用帧获取 ==========
    UFUNCTION(BlueprintCallable, Category = "Camera")
    TArray<uint8> GetFrameAsJPEG(int32 Quality = 85);

    // ========== 录像功能 ==========
    UFUNCTION(BlueprintCallable, Category = "Camera|Recording")
    void StartRecording(float FPS = 30.f);

    UFUNCTION(BlueprintCallable, Category = "Camera|Recording")
    void StopRecording();

    UPROPERTY(BlueprintReadOnly, Category = "Camera|Recording")
    bool bIsRecording = false;

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
    FIntPoint Resolution = FIntPoint(1920, 1080);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float FOV = 90.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.5", ClampMax = "5.0"))
    float BrightnessMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Stream")
    int32 JPEGQuality = 85;

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
    // 录像相关
    FTimerHandle RecordTimerHandle;
    FString RecordingDirectory;
    int32 RecordingFrameIndex = 0;
    void OnRecordTick();

    // TCP 流相关
    FTimerHandle StreamTimerHandle;
    FSocket* ListenSocket = nullptr;
    TArray<FSocket*> ClientSockets;
    void OnStreamTick();
    void AcceptNewClients();
    void SendFrameToClients(const TArray<uint8>& JPEGData);
    void CleanupSockets();
};
