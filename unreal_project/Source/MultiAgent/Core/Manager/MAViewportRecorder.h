// MAViewportRecorder.h
// Viewport 录制管理器 - 录制 UE5 主窗口/上帝视角

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MAViewportRecorder.generated.h"

/**
 * Viewport 录制管理器
 * 
 * 功能:
 * - 录制 UE5 主窗口画面（包括 UI）
 * - 支持设置帧率、分辨率缩放
 * - 保存为 JPEG 序列帧或直接编码为视频
 */
UCLASS()
class MULTIAGENT_API UMAViewportRecorder : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ========== 录制控制 ==========
    
    /** 开始录制 */
    UFUNCTION(BlueprintCallable, Category = "ViewportRecorder")
    void StartRecording(float FPS = 30.f, float ResolutionScale = 1.0f);
    
    /** 停止录制 */
    UFUNCTION(BlueprintCallable, Category = "ViewportRecorder")
    void StopRecording();
    
    /** 切换录制状态 */
    UFUNCTION(BlueprintCallable, Category = "ViewportRecorder")
    void ToggleRecording();
    
    /** 是否正在录制 */
    UFUNCTION(BlueprintPure, Category = "ViewportRecorder")
    bool IsRecording() const { return bIsRecording; }
    
    /** 获取已录制帧数 */
    UFUNCTION(BlueprintPure, Category = "ViewportRecorder")
    int32 GetRecordedFrameCount() const { return RecordingFrameIndex; }
    
    /** 获取录制目录 */
    UFUNCTION(BlueprintPure, Category = "ViewportRecorder")
    FString GetRecordingDirectory() const { return RecordingDirectory; }

    // ========== 录制参数 ==========
    
    /** 录制帧率 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewportRecorder")
    float RecordingFPS = 30.f;
    
    /** 分辨率缩放 (0.5 = 半分辨率, 1.0 = 原始分辨率, 2.0 = 双倍分辨率) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewportRecorder", meta = (ClampMin = "0.25", ClampMax = "4.0"))
    float ResolutionScale = 1.0f;
    
    /** JPEG 质量 (1-100) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewportRecorder", meta = (ClampMin = "1", ClampMax = "100"))
    int32 JPEGQuality = 90;
    
    /** 是否包含 UI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewportRecorder")
    bool bCaptureUI = true;
    
    /** 录制结束后是否自动编码为 MP4 (需要 FFmpeg) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewportRecorder")
    bool bAutoEncodeToMP4 = true;
    
    /** FFmpeg 路径 (留空则使用系统 PATH 中的 ffmpeg) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewportRecorder")
    FString FFmpegPath = TEXT("ffmpeg");

private:
    /** 是否正在录制 */
    bool bIsRecording = false;
    
    /** 录制目录 */
    FString RecordingDirectory;
    
    /** 当前帧索引 */
    int32 RecordingFrameIndex = 0;
    
    /** 录制定时器 */
    FTimerHandle RecordTimerHandle;
    
    /** 录制 Tick */
    void OnRecordTick();
    
    /** 捕获当前帧 */
    void CaptureFrame();
    
    /** 编码为 MP4 视频 */
    void EncodeToMP4();
    
    /** Viewport 绘制完成回调 */
    void OnViewportRendered(FViewport* Viewport, uint32 Unused);
    
    /** 回调句柄 */
    FDelegateHandle ViewportDrawnDelegateHandle;
    
    /** 待保存的帧数据 */
    TArray<FColor> PendingFrameData;
    FIntPoint PendingFrameSize;
    bool bHasPendingFrame = false;
};
