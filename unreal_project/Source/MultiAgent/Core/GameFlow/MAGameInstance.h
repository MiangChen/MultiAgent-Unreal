// MAGameInstance.h
// 游戏实例 - 负责全局状态管理和 Setup 配置

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MAGameInstance.generated.h"

class UMACommSubsystem;
class UMAConfigManager;

/**
 * MultiAgent 游戏实例
 * 
 * 职责:
 * - 提供 ConfigManager 的便捷访问
 * - 管理 Setup 阶段的配置（UI 设置覆盖）
 * - 保持跨关卡的全局状态
 */
UCLASS()
class MULTIAGENT_API UMAGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    //=========================================================================
    // Subsystem 访问
    //=========================================================================

    /** 获取配置管理器 */
    UFUNCTION(BlueprintPure, Category = "Subsystem")
    UMAConfigManager* GetConfigManager() const;

    /** 获取通信子系统 */
    UFUNCTION(BlueprintPure, Category = "Subsystem")
    UMACommSubsystem* GetCommSubsystem() const;

    //=========================================================================
    // Setup 阶段配置 (从 SetupWidget 传入，覆盖 JSON 默认值)
    //=========================================================================

    /** 是否已完成 Setup 配置 */
    UPROPERTY(BlueprintReadOnly, Category = "Setup")
    bool bSetupCompleted = false;

    /** Setup 阶段配置的智能体列表 (类型名 -> 数量) */
    UPROPERTY(BlueprintReadOnly, Category = "Setup")
    TMap<FString, int32> SetupAgentConfigs;

    /** Setup 阶段选择的场景 */
    UPROPERTY(BlueprintReadOnly, Category = "Setup")
    FString SetupSelectedScene;

    /**
     * 保存 Setup 配置
     * @param AgentConfigs 智能体配置 (类型 -> 数量)
     * @param SelectedScene 选择的场景
     */
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void SaveSetupConfig(const TMap<FString, int32>& AgentConfigs, const FString& SelectedScene);

    /** 清除 Setup 配置 */
    UFUNCTION(BlueprintCallable, Category = "Setup")
    void ClearSetupConfig();

    /** 获取 Setup 配置的智能体总数 */
    UFUNCTION(BlueprintPure, Category = "Setup")
    int32 GetSetupTotalAgentCount() const;

    //=========================================================================
    // 静态访问方法
    //=========================================================================

    /**
     * 从任意 UObject 获取 MAGameInstance
     * @param WorldContextObject 世界上下文对象
     * @return MAGameInstance 实例
     */
    UFUNCTION(BlueprintPure, Category = "Game Instance", meta = (WorldContext = "WorldContextObject"))
    static UMAGameInstance* GetMAGameInstance(const UObject* WorldContextObject);

protected:
    virtual void Init() override;
    virtual void Shutdown() override;
};
