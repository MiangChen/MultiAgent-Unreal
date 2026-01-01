// MAEmergencyManager.cpp

#include "MAEmergencyManager.h"
#include "MAAgentManager.h"
#include "../Types/MATypes.h"
#include "../Agent/Character/MACharacter.h"

void UMAEmergencyManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Initialized"));
}

void UMAEmergencyManager::Deinitialize()
{
    // 清理状态
    bIsEventActive = false;
    SourceAgent.Reset();
    
    Super::Deinitialize();
    UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Deinitialized"));
}

void UMAEmergencyManager::TriggerEvent(AMACharacter* InSourceAgent)
{
    // 如果已经有激活的事件，不重复触发
    if (bIsEventActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] Event already active, ignoring TriggerEvent"));
        return;
    }
    
    // 设置 Source Agent
    if (InSourceAgent)
    {
        SourceAgent = InSourceAgent;
    }
    else
    {
        // 自动查找 0 号机器狗
        SourceAgent = FindDefaultSourceAgent();
    }
    
    // 激活事件
    bIsEventActive = true;
    
    // 广播状态变化
    OnEmergencyStateChanged.Broadcast(true);
    
    if (SourceAgent.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Emergency event triggered, SourceAgent: %s"), 
            *SourceAgent->AgentID);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Emergency event triggered, no SourceAgent found"));
    }
}

void UMAEmergencyManager::EndEvent()
{
    // 如果没有激活的事件，不需要结束
    if (!bIsEventActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] No active event, ignoring EndEvent"));
        return;
    }
    
    // 清除状态
    bIsEventActive = false;
    SourceAgent.Reset();
    
    // 广播状态变化
    OnEmergencyStateChanged.Broadcast(false);
    
    UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Emergency event ended"));
}

void UMAEmergencyManager::ToggleEvent()
{
    if (bIsEventActive)
    {
        EndEvent();
    }
    else
    {
        // 键盘模拟场景：使用默认 Agent（0 号机器狗）
        // 未来 Agent 自动触发时，应直接调用 TriggerEvent(Agent) 传入具体 Agent
        TriggerEvent(nullptr);
    }
}

void UMAEmergencyManager::TriggerEventFromAgent(AMACharacter* ReportingAgent)
{
    if (!ReportingAgent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] TriggerEventFromAgent: ReportingAgent is null"));
        return;
    }
    
    // 直接使用报告事件的 Agent 作为 SourceAgent
    TriggerEvent(ReportingAgent);
    
    UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Event triggered by Agent: %s"), *ReportingAgent->AgentID);
}

AMACharacter* UMAEmergencyManager::FindDefaultSourceAgent() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    // 通过 AgentManager 获取 0 号机器狗
    UMAAgentManager* AgentManager = World->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] AgentManager not found"));
        return nullptr;
    }
    
    // 获取所有 Quadruped 类型的 Agent
    TArray<AMACharacter*> Quadrupeds = AgentManager->GetAgentsByType(EMAAgentType::Quadruped);
    
    // 返回第一个（0 号）四足机器人
    if (Quadrupeds.Num() > 0)
    {
        return Quadrupeds[0];
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] No Quadruped found as default SourceAgent"));
    return nullptr;
}
