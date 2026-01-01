// MASquad.cpp
// Squad 实体实现

#include "MASquad.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Character/MAQuadrupedCharacter.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

UMASquad::UMASquad()
{
    SquadID = TEXT("");
    SquadName = TEXT("");
}

// ========== 成员管理 ==========

bool UMASquad::AddMember(AMACharacter* Agent)
{
    if (!Agent)
    {
        return false;
    }

    // 清理无效引用
    CleanupInvalidMembers();

    // 检查是否已经是成员
    if (HasMember(Agent))
    {
        return false;
    }

    // 如果 Agent 在其他 Squad，先离开
    if (Agent->CurrentSquad && Agent->CurrentSquad != this)
    {
        Agent->CurrentSquad->RemoveMember(Agent);
    }

    // 添加成员
    Members.Add(Agent);
    Agent->CurrentSquad = this;

    // 如果没有 Leader，设为 Leader
    if (!Leader.IsValid())
    {
        Leader = Agent;
    }

    UE_LOG(LogTemp, Log, TEXT("[Squad %s] Added member: %s (Total: %d)"),
        *SquadID, *Agent->AgentName, GetMemberCount());

    OnMemberAdded.Broadcast(this, Agent);
    return true;
}

bool UMASquad::RemoveMember(AMACharacter* Agent)
{
    if (!Agent)
    {
        return false;
    }

    // 查找并移除
    int32 RemovedCount = Members.RemoveAll([Agent](const TWeakObjectPtr<AMACharacter>& WeakAgent) {
        return WeakAgent.Get() == Agent;
    });

    if (RemovedCount > 0)
    {
        Agent->CurrentSquad = nullptr;

        // 如果移除的是 Leader，选新 Leader
        if (Leader.Get() == Agent)
        {
            CleanupInvalidMembers();
            Leader = Members.Num() > 0 ? Members[0] : nullptr;
            
            if (Leader.IsValid())
            {
                OnLeaderChanged.Broadcast(this, Leader.Get());
            }
        }

        UE_LOG(LogTemp, Log, TEXT("[Squad %s] Removed member: %s (Remaining: %d)"),
            *SquadID, *Agent->AgentName, GetMemberCount());

        OnMemberRemoved.Broadcast(this, Agent);

        // 如果 Squad 空了，停止编队
        if (IsEmpty())
        {
            StopFormation();
        }

        return true;
    }

    return false;
}

void UMASquad::SetLeader(AMACharacter* NewLeader)
{
    if (!NewLeader || !HasMember(NewLeader))
    {
        return;
    }

    if (Leader.Get() != NewLeader)
    {
        Leader = NewLeader;
        OnLeaderChanged.Broadcast(this, NewLeader);
        
        UE_LOG(LogTemp, Log, TEXT("[Squad %s] New leader: %s"), *SquadID, *NewLeader->AgentName);
    }
}

TArray<AMACharacter*> UMASquad::GetMembers() const
{
    TArray<AMACharacter*> Result;
    for (const TWeakObjectPtr<AMACharacter>& WeakAgent : Members)
    {
        if (AMACharacter* Agent = WeakAgent.Get())
        {
            Result.Add(Agent);
        }
    }
    return Result;
}

int32 UMASquad::GetMemberCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AMACharacter>& WeakAgent : Members)
    {
        if (WeakAgent.IsValid())
        {
            Count++;
        }
    }
    return Count;
}

bool UMASquad::HasMember(AMACharacter* Agent) const
{
    if (!Agent) return false;
    
    for (const TWeakObjectPtr<AMACharacter>& WeakAgent : Members)
    {
        if (WeakAgent.Get() == Agent)
        {
            return true;
        }
    }
    return false;
}

// ========== 编队技能 ==========

void UMASquad::StartFormation(EMAFormationType Type)
{
    if (Type == EMAFormationType::None)
    {
        StopFormation();
        return;
    }

    if (!Leader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Squad %s] Cannot start formation: no leader"), *SquadID);
        return;
    }

    CleanupInvalidMembers();

    if (GetMemberCount() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Squad %s] Cannot start formation: need at least 2 members"), *SquadID);
        return;
    }

    // 停止当前编队
    StopFormation();

    CurrentFormationType = Type;

    UE_LOG(LogTemp, Log, TEXT("[Squad %s] Started %s formation with %d members"),
        *SquadID, *FormationTypeToString(Type), GetMemberCount());

    // 启动定时更新
    if (UWorld* World = WorldPtr.Get())
    {
        World->GetTimerManager().SetTimer(
            FormationTimerHandle,
            this,
            &UMASquad::UpdateFormation,
            FormationUpdateInterval,
            true,
            0.f
        );
    }

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        FString::Printf(TEXT("[%s] %s Formation: %d members"),
            *SquadName, *FormationTypeToString(Type), GetMemberCount()));
}

void UMASquad::StopFormation()
{
    if (CurrentFormationType == EMAFormationType::None)
    {
        return;
    }

    // 停止定时器
    if (UWorld* World = WorldPtr.Get())
    {
        World->GetTimerManager().ClearTimer(FormationTimerHandle);
    }

    // 停止所有成员移动
    for (AMACharacter* Member : GetMembers())
    {
        if (Member && Member != Leader.Get())
        {
            Member->CancelNavigation();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[Squad %s] Stopped formation"), *SquadID);

    CurrentFormationType = EMAFormationType::None;
}

void UMASquad::SetFormationType(EMAFormationType Type)
{
    if (Type == EMAFormationType::None)
    {
        StopFormation();
        return;
    }

    if (CurrentFormationType == EMAFormationType::None)
    {
        StartFormation(Type);
        return;
    }

    CurrentFormationType = Type;
    UpdateFormation();

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
        FString::Printf(TEXT("[%s] Formation: %s"), *SquadName, *FormationTypeToString(Type)));
}

// ========== Squad 技能 ==========

TArray<FString> UMASquad::GetAvailableSkills() const
{
    TArray<FString> Skills;
    
    // 基础技能
    Skills.Add(TEXT("Formation"));
    
    // 未来可以根据成员类型动态添加技能
    // if (HasMemberOfType(EMAAgentType::Quadruped)) Skills.Add(TEXT("Suppress"));
    
    return Skills;
}

bool UMASquad::ExecuteSkill(const FString& SkillName, const FMASquadSkillParams& Params)
{
    if (SkillName.Equals(TEXT("Formation"), ESearchCase::IgnoreCase))
    {
        // 循环切换编队类型
        int32 CurrentIndex = static_cast<int32>(CurrentFormationType);
        int32 NextIndex = (CurrentIndex + 1) % 6;
        SetFormationType(static_cast<EMAFormationType>(NextIndex));
        return true;
    }

    // 未来扩展其他技能
    // if (SkillName.Equals(TEXT("Breach"))) { return ExecuteBreach(Params); }

    UE_LOG(LogTemp, Warning, TEXT("[Squad %s] Unknown skill: %s"), *SquadID, *SkillName);
    return false;
}

// ========== 辅助方法 ==========

FString UMASquad::FormationTypeToString(EMAFormationType Type)
{
    switch (Type)
    {
        case EMAFormationType::None: return TEXT("None");
        case EMAFormationType::Line: return TEXT("Line");
        case EMAFormationType::Column: return TEXT("Column");
        case EMAFormationType::Wedge: return TEXT("Wedge");
        case EMAFormationType::Diamond: return TEXT("Diamond");
        case EMAFormationType::Circle: return TEXT("Circle");
        default: return TEXT("Unknown");
    }
}

void UMASquad::CleanupInvalidMembers()
{
    Members.RemoveAll([](const TWeakObjectPtr<AMACharacter>& WeakAgent) {
        return !WeakAgent.IsValid();
    });
}

// ========== 编队内部实现 ==========

void UMASquad::UpdateFormation()
{
    if (!Leader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Squad %s] Leader lost, stopping formation"), *SquadID);
        StopFormation();
        return;
    }

    CleanupInvalidMembers();

    AMACharacter* LeaderAgent = Leader.Get();
    FVector LeaderLocation = LeaderAgent->GetActorLocation();
    FRotator LeaderRotation = LeaderAgent->GetActorRotation();

    // 获取非 Leader 的成员
    TArray<AMACharacter*> Followers;
    for (AMACharacter* Member : GetMembers())
    {
        if (Member && Member != LeaderAgent)
        {
            Followers.Add(Member);
        }
    }

    if (Followers.Num() == 0)
    {
        return;
    }

    // 计算目标位置
    TArray<FVector> TargetPositions = CalculateFormationPositions(LeaderLocation, LeaderRotation);

    // 更新每个 Follower 的位置
    for (int32 i = 0; i < Followers.Num() && i < TargetPositions.Num(); i++)
    {
        AMACharacter* Follower = Followers[i];
        if (!Follower) continue;

        // 跳过没有能量的成员
        if (!HasEnergy(Follower)) continue;

        FVector CurrentLocation = Follower->GetActorLocation();
        FVector TargetLocation = TargetPositions[i];
        float DistanceToTarget = FVector::Dist2D(CurrentLocation, TargetLocation);

        // 滞后控制
        if (Follower->bIsMoving)
        {
            if (DistanceToTarget < FormationStopMoveThreshold)
            {
                Follower->CancelNavigation();
            }
        }
        else
        {
            if (DistanceToTarget > FormationStartMoveThreshold)
            {
                Follower->TryNavigateTo(TargetLocation);
            }
        }
    }
}

TArray<FVector> UMASquad::CalculateFormationPositions(const FVector& LeaderLocation, const FRotator& LeaderRotation) const
{
    TArray<FVector> Positions;
    
    // 获取非 Leader 的成员数量
    int32 FollowerCount = GetMemberCount() - 1;
    if (FollowerCount <= 0) return Positions;

    for (int32 i = 0; i < FollowerCount; i++)
    {
        FVector LocalOffset = CalculateFormationOffset(i, FollowerCount);
        FVector WorldOffset = LeaderRotation.RotateVector(LocalOffset);
        Positions.Add(LeaderLocation + WorldOffset);
    }

    return Positions;
}

FVector UMASquad::CalculateFormationOffset(int32 MemberIndex, int32 TotalCount) const
{
    FVector Offset = FVector::ZeroVector;

    switch (CurrentFormationType)
    {
        case EMAFormationType::Line:
            // 横向一字排开
            if (MemberIndex % 2 == 0)
                Offset = FVector(0.f, FormationSpacing * ((MemberIndex / 2) + 1), 0.f);
            else
                Offset = FVector(0.f, -FormationSpacing * ((MemberIndex / 2) + 1), 0.f);
            break;

        case EMAFormationType::Column:
            // 纵队
            Offset = FVector(-FormationSpacing * (MemberIndex + 1), 0.f, 0.f);
            break;

        case EMAFormationType::Wedge:
            // V 形
            {
                int32 Row = (MemberIndex / 2) + 1;
                float Side = (MemberIndex % 2 == 0) ? 1.f : -1.f;
                Offset = FVector(-FormationSpacing * Row, FormationSpacing * Row * Side * 0.7f, 0.f);
            }
            break;

        case EMAFormationType::Diamond:
            // 菱形
            switch (MemberIndex % 4)
            {
                case 0: Offset = FVector(-FormationSpacing, 0.f, 0.f); break;
                case 1: Offset = FVector(0.f, FormationSpacing, 0.f); break;
                case 2: Offset = FVector(0.f, -FormationSpacing, 0.f); break;
                case 3: Offset = FVector(FormationSpacing, 0.f, 0.f); break;
            }
            if (MemberIndex >= 4)
            {
                Offset *= (MemberIndex / 4) + 1;
            }
            break;

        case EMAFormationType::Circle:
            // 圆形
            {
                float MinSpacing = FormationSpacing * 0.8f;
                float Radius = FMath::Max(FormationSpacing, (TotalCount * MinSpacing) / (2.f * PI));
                float Angle = (static_cast<float>(MemberIndex) / static_cast<float>(TotalCount)) * 2.f * PI;
                Offset = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
            }
            break;

        default:
            break;
    }

    return Offset;
}

bool UMASquad::HasEnergy(AMACharacter* Agent) const
{
    if (!Agent) return false;

    // 所有 Agent 默认有能量，能量系统由 Charge 技能管理
    return true;
}
