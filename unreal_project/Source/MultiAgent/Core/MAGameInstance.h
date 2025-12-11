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

protected:
    //=========================================================================
    // 生命周期
    //=========================================================================

    /** 初始化 */
    virtual void Init() override;

    /** 关闭 */
    virtual void Shutdown() override;
};
