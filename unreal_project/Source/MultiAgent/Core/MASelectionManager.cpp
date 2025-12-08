// MASelectionManager.cpp
// 星际争霸风格选择系统实现

#include "MASelectionManager.h"
#include "MAAgentManager.h"
#include "../Agent/Character/MACharacter.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void UMASelectionManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("[SelectionManager] Initialized"));
}

void UMASelectionManager::Deinitialize()
{
    ClearSelection();
    ControlGroupsMap.Empty();
    Super::Deinitialize();
}

// ========== 选择操作 ==========

void UMASelectionManager::SelectAgent(AMACharacter* Agent)
{
    if (!Agent) return;

    // 清除之前的选择
    for (AMACharacter* OldAgent : SelectedAgents)
    {
        if (OldAgent)
        {
            UpdateAgentSelectionVisual(OldAgent, false);
        }
    }
    SelectedAgents.Empty();

    // 选中新的
    SelectedAgents.Add(Agent);
    UpdateAgentSelectionVisual(Agent, true);

    BroadcastSelectionChanged();
}

void UMASelectionManager::AddToSelection(AMACharacter* Agent)
{
    if (!Agent) return;
    if (IsSelected(Agent)) return;

    SelectedAgents.Add(Agent);
    UpdateAgentSelectionVisual(Agent, true);

    BroadcastSelectionChanged();
}

void UMASelectionManager::RemoveFromSelection(AMACharacter* Agent)
{
    if (!Agent) return;

    if (SelectedAgents.Remove(Agent) > 0)
    {
        UpdateAgentSelectionVisual(Agent, false);
        BroadcastSelectionChanged();
    }
}

void UMASelectionManager::ToggleSelection(AMACharacter* Agent)
{
    if (!Agent) return;

    if (IsSelected(Agent))
    {
        RemoveFromSelection(Agent);
    }
    else
    {
        AddToSelection(Agent);
    }
}

void UMASelectionManager::SelectAgents(const TArray<AMACharacter*>& Agents)
{
    // 清除之前的选择
    for (AMACharacter* OldAgent : SelectedAgents)
    {
        if (OldAgent)
        {
            UpdateAgentSelectionVisual(OldAgent, false);
        }
    }
    SelectedAgents.Empty();

    // 选中新的
    for (AMACharacter* Agent : Agents)
    {
        if (Agent)
        {
            SelectedAgents.Add(Agent);
            UpdateAgentSelectionVisual(Agent, true);
        }
    }

    BroadcastSelectionChanged();
}

void UMASelectionManager::AddAgentsToSelection(const TArray<AMACharacter*>& Agents)
{
    for (AMACharacter* Agent : Agents)
    {
        if (Agent && !IsSelected(Agent))
        {
            SelectedAgents.Add(Agent);
            UpdateAgentSelectionVisual(Agent, true);
        }
    }

    BroadcastSelectionChanged();
}

void UMASelectionManager::ClearSelection()
{
    for (AMACharacter* Agent : SelectedAgents)
    {
        if (Agent)
        {
            UpdateAgentSelectionVisual(Agent, false);
        }
    }
    SelectedAgents.Empty();

    BroadcastSelectionChanged();
}

void UMASelectionManager::SelectAll()
{
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return;

    SelectAgents(AgentManager->GetAllAgents());
}

// ========== 查询 ==========

bool UMASelectionManager::IsSelected(AMACharacter* Agent) const
{
    return Agent && SelectedAgents.Contains(Agent);
}

// ========== 编组操作 ==========

void UMASelectionManager::SaveToControlGroup(int32 GroupIndex)
{
    if (GroupIndex < 0 || GroupIndex >= MAX_CONTROL_GROUPS) return;

    ControlGroupsMap.Add(GroupIndex, SelectedAgents);

    UE_LOG(LogTemp, Log, TEXT("[SelectionManager] Saved %d agents to group %d"),
        SelectedAgents.Num(), GroupIndex);

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
        FString::Printf(TEXT("Group %d: %d agents"), GroupIndex, SelectedAgents.Num()));

    OnControlGroupChanged.Broadcast(GroupIndex, ControlGroupsMap[GroupIndex]);
}

void UMASelectionManager::SelectControlGroup(int32 GroupIndex)
{
    if (GroupIndex < 0 || GroupIndex >= MAX_CONTROL_GROUPS) return;

    TArray<AMACharacter*>* GroupPtr = ControlGroupsMap.Find(GroupIndex);
    if (!GroupPtr || GroupPtr->Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
            FString::Printf(TEXT("Group %d is empty"), GroupIndex));
        return;
    }

    CleanupInvalidAgents(*GroupPtr);

    if (GroupPtr->Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
            FString::Printf(TEXT("Group %d is empty"), GroupIndex));
        return;
    }

    SelectAgents(*GroupPtr);

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
        FString::Printf(TEXT("Selected group %d: %d agents"), GroupIndex, SelectedAgents.Num()));
}

void UMASelectionManager::AddControlGroupToSelection(int32 GroupIndex)
{
    if (GroupIndex < 0 || GroupIndex >= MAX_CONTROL_GROUPS) return;

    TArray<AMACharacter*>* GroupPtr = ControlGroupsMap.Find(GroupIndex);
    if (GroupPtr)
    {
        CleanupInvalidAgents(*GroupPtr);
        AddAgentsToSelection(*GroupPtr);
    }
}

TArray<AMACharacter*> UMASelectionManager::GetControlGroup(int32 GroupIndex) const
{
    if (GroupIndex < 0 || GroupIndex >= MAX_CONTROL_GROUPS)
    {
        return TArray<AMACharacter*>();
    }
    
    const TArray<AMACharacter*>* GroupPtr = ControlGroupsMap.Find(GroupIndex);
    return GroupPtr ? *GroupPtr : TArray<AMACharacter*>();
}

void UMASelectionManager::ClearControlGroup(int32 GroupIndex)
{
    if (GroupIndex < 0 || GroupIndex >= MAX_CONTROL_GROUPS) return;

    ControlGroupsMap.Remove(GroupIndex);
    OnControlGroupChanged.Broadcast(GroupIndex, TArray<AMACharacter*>());
}

bool UMASelectionManager::IsControlGroupEmpty(int32 GroupIndex) const
{
    if (GroupIndex < 0 || GroupIndex >= MAX_CONTROL_GROUPS) return true;
    
    const TArray<AMACharacter*>* GroupPtr = ControlGroupsMap.Find(GroupIndex);
    return !GroupPtr || GroupPtr->Num() == 0;
}

FVector UMASelectionManager::GetControlGroupCenter(int32 GroupIndex) const
{
    if (GroupIndex < 0 || GroupIndex >= MAX_CONTROL_GROUPS)
    {
        return FVector::ZeroVector;
    }

    const TArray<AMACharacter*>* GroupPtr = ControlGroupsMap.Find(GroupIndex);
    if (!GroupPtr) return FVector::ZeroVector;

    FVector Center = FVector::ZeroVector;
    int32 Count = 0;

    for (AMACharacter* Agent : *GroupPtr)
    {
        if (Agent)
        {
            Center += Agent->GetActorLocation();
            Count++;
        }
    }

    return Count > 0 ? Center / Count : FVector::ZeroVector;
}

FVector UMASelectionManager::GetSelectionCenter() const
{
    FVector Center = FVector::ZeroVector;
    int32 Count = 0;

    for (AMACharacter* Agent : SelectedAgents)
    {
        if (Agent)
        {
            Center += Agent->GetActorLocation();
            Count++;
        }
    }

    return Count > 0 ? Center / Count : FVector::ZeroVector;
}

// ========== 框选辅助 ==========

TArray<AMACharacter*> UMASelectionManager::GetAgentsInScreenRect(
    const FVector2D& StartPos, 
    const FVector2D& EndPos, 
    APlayerController* PC) const
{
    TArray<AMACharacter*> Result;

    if (!PC) return Result;

    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager) return Result;

    // 计算矩形边界
    float MinX = FMath::Min(StartPos.X, EndPos.X);
    float MaxX = FMath::Max(StartPos.X, EndPos.X);
    float MinY = FMath::Min(StartPos.Y, EndPos.Y);
    float MaxY = FMath::Max(StartPos.Y, EndPos.Y);

    // 检查每个 Agent 是否在矩形内
    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (!Agent) continue;

        FVector2D ScreenPos;
        if (PC->ProjectWorldLocationToScreen(Agent->GetActorLocation(), ScreenPos))
        {
            if (ScreenPos.X >= MinX && ScreenPos.X <= MaxX &&
                ScreenPos.Y >= MinY && ScreenPos.Y <= MaxY)
            {
                Result.Add(Agent);
            }
        }
    }

    return Result;
}

// ========== 内部方法 ==========

void UMASelectionManager::CleanupInvalidAgents(TArray<AMACharacter*>& Agents)
{
    Agents.RemoveAll([](AMACharacter* Agent) {
        return !IsValid(Agent);
    });
}

void UMASelectionManager::UpdateAgentSelectionVisual(AMACharacter* Agent, bool bSelected)
{
    if (!Agent) return;

    // TODO: 更新 Agent 的选中视觉效果
    // 可以是：
    // - 脚下的选择圈
    // - 描边效果
    // - 头顶图标
    
    // 暂时用 ShowStatus 显示
    if (bSelected)
    {
        Agent->ShowStatus(TEXT("[Selected]"), 0.5f);
    }
}

void UMASelectionManager::BroadcastSelectionChanged()
{
    OnSelectionChanged.Broadcast(SelectedAgents);
}
