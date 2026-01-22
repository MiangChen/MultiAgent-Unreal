// MAEmergencyManager.cpp

#include "MAEmergencyManager.h"
#include "MAAgentManager.h"
#include "../Types/MATypes.h"
#include "../Agent/Character/MACharacter.h"
#include "../../UI/Modal/MAEmergencyModal.h"

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
    CurrentEventData = FMAEmergencyEventData();
    
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
    
    // 清除当前事件数据
    CurrentEventData = FMAEmergencyEventData();
    
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

void UMAEmergencyManager::TriggerEventWithData(const FMAEmergencyEventData& EventData)
{
    // Requirements: 3.1, 3.4
    // 如果已经有激活的事件，不重复触发
    if (bIsEventActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] Event already active, ignoring TriggerEventWithData"));
        return;
    }
    
    // 存储完整事件数据
    CurrentEventData = EventData;
    
    // 应用默认值确保数据完整性
    CurrentEventData.ApplyDefaults();
    
    UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] TriggerEventWithData: Looking for Agent with ID=%s"),
        *CurrentEventData.SourceAgentId);
    
    // 尝试根据 SourceAgentId 查找对应的 Agent
    if (!CurrentEventData.SourceAgentId.IsEmpty())
    {
        UWorld* World = GetWorld();
        if (World)
        {
            UMAAgentManager* AgentManager = World->GetSubsystem<UMAAgentManager>();
            if (AgentManager)
            {
                AMACharacter* FoundAgent = AgentManager->GetAgentByID(CurrentEventData.SourceAgentId);
                if (FoundAgent)
                {
                    SourceAgent = FoundAgent;
                    UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Found Agent by ID: %s, HasCamera=%s"),
                        *FoundAgent->AgentID,
                        FoundAgent->GetCameraSensor() ? TEXT("Yes") : TEXT("No"));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] Agent not found by ID: %s"), 
                        *CurrentEventData.SourceAgentId);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] AgentManager not found"));
            }
        }
    }
    
    // 如果没有找到 Agent，使用默认 Agent
    if (!SourceAgent.IsValid())
    {
        SourceAgent = FindDefaultSourceAgent();
        if (SourceAgent.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Using default Agent: %s, HasCamera=%s"),
                *SourceAgent->AgentID,
                SourceAgent->GetCameraSensor() ? TEXT("Yes") : TEXT("No"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] No default Agent found"));
        }
    }
    
    // 激活事件
    bIsEventActive = true;
    
    // 广播状态变化委托
    OnEmergencyStateChanged.Broadcast(true);
    
    // 广播完整事件数据委托 (Requirements: 3.4)
    OnEmergencyEventReceived.Broadcast(CurrentEventData);
    
    UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Emergency event triggered with data - AgentId=%s, EventType=%s, Options=%d, SourceAgent=%s"),
        *CurrentEventData.SourceAgentId, *CurrentEventData.EventType, CurrentEventData.AvailableOptions.Num(),
        SourceAgent.IsValid() ? *SourceAgent->AgentID : TEXT("None"));
}

void UMAEmergencyManager::TriggerEventFromAgentWithData(
    AMACharacter* ReportingAgent,
    const FString& InEventType,
    const FString& InDescription,
    const TArray<FString>& InAvailableOptions)
{
    // Requirements: 3.1, 3.2, 3.3
    if (!ReportingAgent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] TriggerEventFromAgentWithData: ReportingAgent is null"));
        return;
    }
    
    // 如果已经有激活的事件，不重复触发
    if (bIsEventActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("[EmergencyManager] Event already active, ignoring TriggerEventFromAgentWithData"));
        return;
    }
    
    // 创建事件数据并从 Agent 填充信息 (Requirements: 3.2, 3.3)
    FMAEmergencyEventData EventData;
    
    // 从 Agent 自动填充 SourceAgentId
    EventData.SourceAgentId = ReportingAgent->AgentID;
    
    // 从 Agent 自动填充 SourceAgentName
    EventData.SourceAgentName = ReportingAgent->GetName();
    
    // 从 Agent 自动填充 Location
    EventData.Location = ReportingAgent->GetActorLocation();
    
    // 设置事件类型和描述
    EventData.EventType = InEventType;
    EventData.Description = InDescription;
    
    // 设置可用选项
    EventData.AvailableOptions = InAvailableOptions;
    
    // 设置时间戳
    EventData.Timestamp = FDateTime::Now();
    
    // 应用默认值确保数据完整性
    EventData.ApplyDefaults();
    
    // 存储事件数据
    CurrentEventData = EventData;
    
    // 设置 Source Agent
    SourceAgent = ReportingAgent;
    
    // 激活事件
    bIsEventActive = true;
    
    // 广播状态变化委托
    OnEmergencyStateChanged.Broadcast(true);
    
    // 广播完整事件数据委托 (Requirements: 3.4)
    OnEmergencyEventReceived.Broadcast(CurrentEventData);
    
    UE_LOG(LogTemp, Log, TEXT("[EmergencyManager] Event triggered by Agent with data: %s, EventType=%s, Options=%d"),
        *ReportingAgent->AgentID, *InEventType, InAvailableOptions.Num());
}

FMAEmergencyEventData UMAEmergencyManager::GetCurrentEventData() const
{
    return CurrentEventData;
}
