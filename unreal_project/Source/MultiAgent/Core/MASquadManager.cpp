// MASquadManager.cpp
// Squad 管理器实现

#include "MASquadManager.h"
#include "MASquad.h"
#include "../Agent/Character/MACharacter.h"
#include "Engine/Engine.h"

void UMASquadManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("[SquadManager] Initialized"));
}

void UMASquadManager::Deinitialize()
{
    DisbandAllSquads();
    Super::Deinitialize();
}

// ========== Squad 生命周期 ==========

UMASquad* UMASquadManager::CreateSquad(const TArray<AMACharacter*>& Members, 
                                        AMACharacter* Leader,
                                        const FString& SquadName)
{
    if (Members.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SquadManager] Cannot create empty squad"));
        return nullptr;
    }

    // 创建 Squad 对象
    UMASquad* NewSquad = NewObject<UMASquad>(this);
    NewSquad->SquadID = GenerateSquadID();
    NewSquad->SquadName = SquadName.IsEmpty() ? NewSquad->SquadID : SquadName;
    NewSquad->SetWorld(GetWorld());

    // 添加成员
    for (AMACharacter* Agent : Members)
    {
        if (Agent)
        {
            NewSquad->AddMember(Agent);
        }
    }

    // 设置 Leader
    if (Leader && NewSquad->HasMember(Leader))
    {
        NewSquad->SetLeader(Leader);
    }
    else if (NewSquad->GetMemberCount() > 0)
    {
        // 默认第一个成员为 Leader
        NewSquad->SetLeader(NewSquad->GetMembers()[0]);
    }

    Squads.Add(NewSquad);

    UE_LOG(LogTemp, Log, TEXT("[SquadManager] Created Squad '%s' with %d members, Leader: %s"),
        *NewSquad->SquadName,
        NewSquad->GetMemberCount(),
        NewSquad->GetLeader() ? *NewSquad->GetLeader()->AgentName : TEXT("None"));

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        FString::Printf(TEXT("Squad '%s' created: %d members"),
            *NewSquad->SquadName, NewSquad->GetMemberCount()));

    OnSquadCreated.Broadcast(NewSquad);
    return NewSquad;
}

bool UMASquadManager::DisbandSquad(UMASquad* Squad)
{
    if (!Squad)
    {
        return false;
    }

    // 停止编队
    Squad->StopFormation();

    // 移除所有成员（让他们变回独立）
    TArray<AMACharacter*> Members = Squad->GetMembers();
    for (AMACharacter* Member : Members)
    {
        if (Member)
        {
            Member->CurrentSquad = nullptr;
        }
    }

    // 从列表移除
    if (Squads.Remove(Squad) > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[SquadManager] Disbanded Squad '%s'"), *Squad->SquadName);

        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
            FString::Printf(TEXT("Squad '%s' disbanded"), *Squad->SquadName));

        OnSquadDisbanded.Broadcast(Squad);
        return true;
    }

    return false;
}

void UMASquadManager::DisbandAllSquads()
{
    TArray<UMASquad*> SquadsCopy = Squads;
    for (UMASquad* Squad : SquadsCopy)
    {
        DisbandSquad(Squad);
    }
    Squads.Empty();
}

// ========== 查询 ==========

UMASquad* UMASquadManager::GetSquadByID(const FString& SquadID) const
{
    for (UMASquad* Squad : Squads)
    {
        if (Squad && Squad->SquadID == SquadID)
        {
            return Squad;
        }
    }
    return nullptr;
}

UMASquad* UMASquadManager::GetSquadByName(const FString& SquadName) const
{
    for (UMASquad* Squad : Squads)
    {
        if (Squad && Squad->SquadName == SquadName)
        {
            return Squad;
        }
    }
    return nullptr;
}

UMASquad* UMASquadManager::GetSquadByAgent(AMACharacter* Agent) const
{
    if (!Agent)
    {
        return nullptr;
    }

    // 直接从 Agent 获取（更快）
    return Agent->CurrentSquad;
}

// ========== 快捷方法 ==========

UMASquad* UMASquadManager::QuickFormSquad(const TArray<AMACharacter*>& SelectedAgents)
{
    if (SelectedAgents.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SquadManager] Need at least 2 agents to form squad"));
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange,
            TEXT("Need at least 2 agents to form squad"));
        return nullptr;
    }

    // 生成名称
    FString SquadName = FString::Printf(TEXT("Squad_%d"), NextSquadIndex);

    // 第一个选中的作为 Leader
    return CreateSquad(SelectedAgents, SelectedAgents[0], SquadName);
}

bool UMASquadManager::LeaveSquad(AMACharacter* Agent)
{
    if (!Agent || !Agent->CurrentSquad)
    {
        return false;
    }

    UMASquad* Squad = Agent->CurrentSquad;
    bool bRemoved = Squad->RemoveMember(Agent);

    // 如果 Squad 空了，自动解散
    if (Squad->IsEmpty())
    {
        DisbandSquad(Squad);
    }

    return bRemoved;
}

// ========== 内部方法 ==========

FString UMASquadManager::GenerateSquadID()
{
    return FString::Printf(TEXT("squad_%d"), NextSquadIndex++);
}

void UMASquadManager::CleanupEmptySquads()
{
    Squads.RemoveAll([this](UMASquad* Squad) {
        if (Squad && Squad->IsEmpty())
        {
            OnSquadDisbanded.Broadcast(Squad);
            return true;
        }
        return false;
    });
}
