// MAEmergencyManager.h
// 突发事件管理器 - 负责管理突发事件的触发、状态维护和通知
// 作为 UWorldSubsystem 管理所有突发事件

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MAEmergencyManager.generated.h"

class AMACharacter;

// 事件状态变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEmergencyStateChanged, bool, bIsActive);

/**
 * 突发事件管理器
 * 
 * 功能:
 * - 管理突发事件的触发和结束
 * - 维护事件激活状态
 * - 记录触发事件的 Source Agent (默认为 0 号机器狗)
 * - 广播事件状态变化
 * 
 * 使用方式:
 *   MA_SUBS.EmergencyManager->TriggerEvent();
 *   MA_SUBS.EmergencyManager->EndEvent();
 *   MA_SUBS.EmergencyManager->ToggleEvent();
 */
UCLASS()
class MULTIAGENT_API UMAEmergencyManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ========== 事件控制 ==========
    
    /**
     * 触发突发事件
     * @param SourceAgent 触发事件的 Agent，如果为 nullptr 则自动查找 0 号机器狗
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency")
    void TriggerEvent(AMACharacter* SourceAgent = nullptr);
    
    /**
     * 结束突发事件
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency")
    void EndEvent();
    
    /**
     * 切换事件状态（触发或结束）
     * 如果当前无事件则触发（使用默认 Agent），如果有事件则结束
     * 主要用于键盘模拟场景
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency")
    void ToggleEvent();
    
    /**
     * Agent 主动报告突发事件
     * 未来 Agent 自动触发事件时使用此方法
     * @param ReportingAgent 报告事件的 Agent，其相机将显示在详情界面
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency")
    void TriggerEventFromAgent(AMACharacter* ReportingAgent);

    // ========== 状态查询 ==========
    
    /**
     * 查询事件是否激活
     * @return 如果有激活的突发事件返回 true
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency")
    bool IsEventActive() const { return bIsEventActive; }
    
    /**
     * 获取触发事件的 Source Agent
     * @return 触发事件的 Agent，如果无事件或 Agent 已销毁则返回 nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "Emergency")
    AMACharacter* GetSourceAgent() const { return SourceAgent.Get(); }

    // ========== 委托 ==========
    
    /**
     * 事件状态变化委托
     * 当事件触发或结束时广播
     * @param bIsActive 新的事件状态
     */
    UPROPERTY(BlueprintAssignable, Category = "Emergency")
    FOnEmergencyStateChanged OnEmergencyStateChanged;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    // 事件激活状态
    bool bIsEventActive = false;
    
    // 触发事件的 Agent (使用弱引用防止悬空指针)
    TWeakObjectPtr<AMACharacter> SourceAgent;
    
    /**
     * 查找 0 号机器狗作为默认 Source Agent
     * @return 找到的机器狗，如果不存在返回 nullptr
     */
    AMACharacter* FindDefaultSourceAgent() const;
};
