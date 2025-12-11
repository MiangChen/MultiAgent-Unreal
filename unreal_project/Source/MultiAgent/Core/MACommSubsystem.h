// MACommSubsystem.h
// 通信子系统 - 负责与外部规划器通信
// Requirements: 4.1, 4.2, 4.3, 4.4

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MASimTypes.h"
#include "MACommSubsystem.generated.h"

/**
 * 通信子系统
 * 
 * 职责:
 * - 与外部规划器通信 (HTTP)
 * - 发送自然语言指令
 * - 广播规划器响应给订阅者
 * - 支持模拟数据模式用于开发测试
 * 
 * Requirements: 4.1, 4.2, 4.3, 4.4
 */
UCLASS()
class MULTIAGENT_API UMACommSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 配置
    //=========================================================================

    /** 规划器服务器 URL */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
    FString ServerURL = TEXT("http://localhost:8080");

    /** 是否使用模拟数据 (开发测试用) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
    bool bUseMockData = true;

    //=========================================================================
    // 命令发送
    //=========================================================================

    /**
     * 发送自然语言指令
     * 根据 bUseMockData 设置决定是发送真实 HTTP 请求还是生成模拟响应
     * @param Command 用户输入的自然语言指令
     * Requirements: 4.2
     */
    UFUNCTION(BlueprintCallable, Category = "Communication")
    void SendNaturalLanguageCommand(const FString& Command);

    //=========================================================================
    // 事件委托
    //=========================================================================

    /** 
     * 规划器响应委托 - 当收到规划器响应时广播
     * Requirements: 4.3
     */
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnMAPlannerResponse OnPlannerResponse;

    //=========================================================================
    // 状态查询
    //=========================================================================

    /** 是否正在等待响应 */
    UFUNCTION(BlueprintPure, Category = "Status")
    bool IsWaitingForResponse() const { return bWaitingForResponse; }

    /** 获取最后发送的指令 */
    UFUNCTION(BlueprintPure, Category = "Status")
    FString GetLastCommand() const { return LastCommand; }

protected:
    //=========================================================================
    // 生命周期
    //=========================================================================

    /** 初始化子系统 - Requirements: 4.1 */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    
    /** 反初始化子系统 */
    virtual void Deinitialize() override;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /** 
     * 生成模拟响应 (开发测试用)
     * 根据用户指令生成合理的模拟规划结果
     * @param UserCommand 用户输入的指令
     * Requirements: 4.4
     */
    void GenerateMockPlanResponse(const FString& UserCommand);

    /**
     * 广播规划器响应给所有订阅者
     * @param Response 规划器响应数据
     * Requirements: 4.3
     */
    void BroadcastResponse(const FMAPlannerResponse& Response);

private:
    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 是否正在等待响应 */
    bool bWaitingForResponse = false;

    /** 最后发送的指令 */
    FString LastCommand;
};
