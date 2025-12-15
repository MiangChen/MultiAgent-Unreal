// MAGameInstance.h
// 游戏实例 - 负责全局配置和跨关卡状态管理
// Requirements: 8.3, 8.4

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MAGameInstance.generated.h"

// 前向声明
class UMACommSubsystem;

/**
 * MultiAgent 游戏实例
 * 
 * 职责:
 * - 管理全局配置 (服务器地址、调试模式等)
 * - 从 JSON 配置文件加载设置
 * - 支持 JSON 指定启动地图
 * - 提供 CommSubsystem 的便捷访问
 * - 保持跨关卡的全局状态
 * 
 * Requirements: 8.3, 8.4
 */
UCLASS()
class MULTIAGENT_API UMAGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UMAGameInstance();

    //=========================================================================
    // 全局配置
    //=========================================================================

    /** 规划器服务器 URL */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Connection")
    FString PlannerServerURL = TEXT("http://localhost:8080");

    /** 是否使用模拟数据 (开发测试用) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Debug")
    bool bUseMockData = true;

    /** 是否启用调试模式 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Debug")
    bool bDebugMode = true;

    //=========================================================================
    // 轮询配置
    // Requirements: 5.5, 8.1
    //=========================================================================

    /** 是否启用轮询 (默认 true) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Polling")
    bool bEnablePolling = true;

    /** 轮询间隔 (秒，默认 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Polling")
    float PollIntervalSeconds = 1.0f;

    /** JSON 配置中指定的默认地图路径 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Map")
    FString ConfiguredMapPath;

    /** 是否已加载配置地图 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Map")
    bool bHasLoadedConfiguredMap = false;

    //=========================================================================
    // Spectator 配置
    //=========================================================================

    /** Spectator 初始位置 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Spectator")
    FVector SpectatorStartPosition = FVector(0, 0, 500);

    /** Spectator 初始旋转 */
    UPROPERTY(BlueprintReadOnly, Category = "Config|Spectator")
    FRotator SpectatorStartRotation = FRotator(-45, 0, 0);

    //=========================================================================
    // Subsystem 访问
    //=========================================================================

    /**
     * 获取通信子系统
     * @return CommSubsystem 实例，如果不存在则返回 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "Subsystem")
    UMACommSubsystem* GetCommSubsystem() const;

    //=========================================================================
    // 静态访问方法
    //=========================================================================

    /**
     * 从任意 UObject 获取 MAGameInstance
     * @param WorldContextObject 世界上下文对象
     * @return MAGameInstance 实例，如果不存在则返回 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "Game Instance", meta = (WorldContext = "WorldContextObject"))
    static UMAGameInstance* GetMAGameInstance(const UObject* WorldContextObject);

    //=========================================================================
    // 地图加载
    //=========================================================================

    /**
     * 加载 JSON 配置中指定的地图
     * 在首次进入游戏时调用
     */
    UFUNCTION(BlueprintCallable, Category = "Map")
    void LoadConfiguredMap();

protected:
    //=========================================================================
    // 生命周期
    //=========================================================================

    /** 初始化 */
    virtual void Init() override;

    /** 关闭 */
    virtual void Shutdown() override;

    /** 从 JSON 文件加载配置 */
    void LoadConfigFromJSON();
};
