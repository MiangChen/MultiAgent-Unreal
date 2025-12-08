// MACommandManager.cpp
// RTS 风格命令管理器实现

#include "MACommandManager.h"
#include "MAAgentManager.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Character/MARobotDogCharacter.h"
#include "../Agent/Character/MADroneCharacter.h"
#include "../Agent/GAS/MAAbilitySystemComponent.h"
#include "../Environment/MAPatrolPath.h"
#include "../Environment/MACoverageArea.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void UMACommandManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    InitializeCommandTags();
    UE_LOG(LogTemp, Log, TEXT("[CommandManager] Initialized"));
}

void UMACommandManager::InitializeCommandTags()
{
    // 缓存所有命令对应的 GameplayTag，避免运行时重复创建
    CommandTagCache.Add(EMACommand::Idle, FGameplayTag::RequestGameplayTag(FName("Command.Idle")));
    CommandTagCache.Add(EMACommand::Patrol, FGameplayTag::RequestGameplayTag(FName("Command.Patrol")));
    CommandTagCache.Add(EMACommand::Charge, FGameplayTag::RequestGameplayTag(FName("Command.Charge")));
    CommandTagCache.Add(EMACommand::Follow, FGameplayTag::RequestGameplayTag(FName("Command.Follow")));
    CommandTagCache.Add(EMACommand::Coverage, FGameplayTag::RequestGameplayTag(FName("Command.Coverage")));
    // Avoid 和 Navigate 是直接激活 Ability，不使用 Command Tag
}

// ========== 单个 Agent 命令 ==========

bool UMACommandManager::SendCommandToAgent(AMACharacter* Agent, EMACommand Command, const FMACommandParams& Params)
{
    if (!Agent)
    {
        return false;
    }

    bool bSuccess = ApplyCommand(Agent, Command, Params);
    
    if (bSuccess && Params.bShowMessage)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("%s: %s"), *Agent->AgentName, *CommandToString(Command)));
    }

    return bSuccess;
}

// ========== 批量命令 ==========

FMACommandResult UMACommandManager::SendCommand(EMACommand Command, const FMACommandParams& InParams)
{
    // 自动填充缺失的参数
    FMACommandParams Params = InParams;
    AutoFillCommandParams(Command, Params);
    
    TArray<AMACharacter*> Agents = GetControllableAgents(Params.bExcludeTracker);
    return SendCommandToAgents(Agents, Command, Params);
}

FMACommandResult UMACommandManager::SendCommandToType(EMAAgentType Type, EMACommand Command, const FMACommandParams& Params)
{
    FMACommandResult Result;
    
    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        Result.Message = TEXT("No AgentManager");
        return Result;
    }

    TArray<AMACharacter*> Agents = AgentManager->GetAgentsByType(Type);
    return SendCommandToAgents(Agents, Command, Params);
}

FMACommandResult UMACommandManager::SendCommandToAgents(const TArray<AMACharacter*>& Agents, EMACommand Command, const FMACommandParams& Params)
{
    FMACommandResult Result;

    if (Agents.Num() == 0)
    {
        Result.Message = TEXT("No agents to command");
        if (Params.bShowMessage)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, Result.Message);
        }
        return Result;
    }

    for (AMACharacter* Agent : Agents)
    {
        if (!Agent) continue;
        if (ShouldExcludeAgent(Agent, Params)) continue;

        if (ApplyCommand(Agent, Command, Params))
        {
            Result.AffectedCount++;
            
            if (Params.bShowMessage)
            {
                FColor MessageColor = FColor::Green;
                switch (Command)
                {
                    case EMACommand::Idle: MessageColor = FColor::White; break;
                    case EMACommand::Patrol: MessageColor = FColor::Green; break;
                    case EMACommand::Charge: MessageColor = FColor::Yellow; break;
                    case EMACommand::Follow: MessageColor = FColor::Cyan; break;
                    case EMACommand::Coverage: MessageColor = FColor::Magenta; break;
                    case EMACommand::Avoid: MessageColor = FColor::Orange; break;
                    default: break;
                }
                
                GEngine->AddOnScreenDebugMessage(-1, 2.f, MessageColor,
                    FString::Printf(TEXT("%s: %s"), *Agent->AgentName, *CommandToString(Command)));
            }
        }
    }

    Result.bSuccess = Result.AffectedCount > 0;
    Result.Message = FString::Printf(TEXT("%s: %d agents"), *CommandToString(Command), Result.AffectedCount);

    // 广播命令事件
    OnCommandSent.Broadcast(Command, Result.AffectedCount, Params);

    UE_LOG(LogTemp, Log, TEXT("[CommandManager] %s"), *Result.Message);

    return Result;
}

// ========== 查询方法 ==========

TArray<AMACharacter*> UMACommandManager::GetControllableAgents(bool bExcludeTracker) const
{
    TArray<AMACharacter*> Result;

    UMAAgentManager* AgentManager = GetWorld()->GetSubsystem<UMAAgentManager>();
    if (!AgentManager)
    {
        return Result;
    }

    // 收集 RobotDog
    Result.Append(AgentManager->GetAgentsByType(EMAAgentType::RobotDog));
    
    // 收集所有 Drone 类型
    Result.Append(AgentManager->GetAllDrones());

    // 排除 Tracker
    if (bExcludeTracker)
    {
        Result.RemoveAll([](AMACharacter* Agent) {
            return Agent && Agent->AgentName.Contains(TEXT("Tracker"));
        });
    }

    return Result;
}

EMACommand UMACommandManager::GetAgentCurrentCommand(AMACharacter* Agent) const
{
    if (!Agent)
    {
        return EMACommand::None;
    }

    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Agent->GetAbilitySystemComponent());
    if (!ASC)
    {
        return EMACommand::None;
    }

    // 检查当前拥有的命令 Tag
    for (const auto& Pair : CommandTagCache)
    {
        if (ASC->HasMatchingGameplayTag(Pair.Value))
        {
            return Pair.Key;
        }
    }

    return EMACommand::None;
}

// ========== 辅助方法 ==========

FString UMACommandManager::CommandToString(EMACommand Command)
{
    switch (Command)
    {
        case EMACommand::None: return TEXT("None");
        case EMACommand::Idle: return TEXT("Idle");
        case EMACommand::Patrol: return TEXT("Patrol");
        case EMACommand::Charge: return TEXT("Charge");
        case EMACommand::Follow: return TEXT("Follow");
        case EMACommand::Coverage: return TEXT("Coverage");
        case EMACommand::Avoid: return TEXT("Avoid");
        case EMACommand::Navigate: return TEXT("Navigate");
        default: return TEXT("Unknown");
    }
}

EMACommand UMACommandManager::StringToCommand(const FString& CommandString)
{
    if (CommandString.Equals(TEXT("Idle"), ESearchCase::IgnoreCase)) return EMACommand::Idle;
    if (CommandString.Equals(TEXT("Patrol"), ESearchCase::IgnoreCase)) return EMACommand::Patrol;
    if (CommandString.Equals(TEXT("Charge"), ESearchCase::IgnoreCase)) return EMACommand::Charge;
    if (CommandString.Equals(TEXT("Follow"), ESearchCase::IgnoreCase)) return EMACommand::Follow;
    if (CommandString.Equals(TEXT("Coverage"), ESearchCase::IgnoreCase)) return EMACommand::Coverage;
    if (CommandString.Equals(TEXT("Avoid"), ESearchCase::IgnoreCase)) return EMACommand::Avoid;
    if (CommandString.Equals(TEXT("Navigate"), ESearchCase::IgnoreCase)) return EMACommand::Navigate;
    return EMACommand::None;
}

// ========== 内部实现 ==========

bool UMACommandManager::ApplyCommand(AMACharacter* Agent, EMACommand Command, const FMACommandParams& Params)
{
    if (!Agent)
    {
        return false;
    }

    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Agent->GetAbilitySystemComponent());
    
    // Navigate 和 Avoid 是直接操作，不需要 ASC Tag
    if (Command == EMACommand::Navigate)
    {
        return Agent->TryNavigateTo(Params.TargetLocation);
    }
    
    if (Command == EMACommand::Avoid)
    {
        if (!ASC)
        {
            return false;
        }
        
        // Drone 保持高度
        FVector FinalTarget = Params.TargetLocation;
        if (Agent->AgentType == EMAAgentType::Drone ||
            Agent->AgentType == EMAAgentType::DronePhantom4 ||
            Agent->AgentType == EMAAgentType::DroneInspire2)
        {
            FinalTarget.Z = Agent->GetActorLocation().Z;
        }
        
        return ASC->TryActivateAvoid(FinalTarget);
    }

    // 其他命令需要 ASC
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CommandManager] %s has no ASC"), *Agent->AgentName);
        return false;
    }

    // 设置 Agent 属性 (PatrolPath, FollowTarget 等)
    SetAgentCommandProperties(Agent, Command, Params);

    // 清除其他命令 Tag
    ClearOtherCommandTags(ASC, Command);

    // 添加新命令 Tag
    FGameplayTag CommandTag = CommandToTag(Command);
    if (CommandTag.IsValid())
    {
        ASC->AddLooseGameplayTag(CommandTag);
    }

    // Idle 命令额外取消正在执行的 Ability
    if (Command == EMACommand::Idle)
    {
        ASC->CancelAvoid();
        ASC->CancelNavigate();
    }

    return true;
}

void UMACommandManager::ClearOtherCommandTags(UMAAbilitySystemComponent* ASC, EMACommand ExceptCommand)
{
    if (!ASC)
    {
        return;
    }

    for (const auto& Pair : CommandTagCache)
    {
        if (Pair.Key != ExceptCommand && Pair.Value.IsValid())
        {
            ASC->RemoveLooseGameplayTag(Pair.Value);
        }
    }
}

void UMACommandManager::SetAgentCommandProperties(AMACharacter* Agent, EMACommand Command, const FMACommandParams& Params)
{
    if (!Agent)
    {
        return;
    }

    switch (Command)
    {
        case EMACommand::Patrol:
            if (Params.PatrolPath.IsValid())
            {
                // 通过 Interface 设置 PatrolPath
                if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Agent))
                {
                    Robot->SetPatrolPath(Params.PatrolPath.Get());
                }
                else if (AMADroneCharacter* Drone = Cast<AMADroneCharacter>(Agent))
                {
                    Drone->SetPatrolPath(Params.PatrolPath.Get());
                }
            }
            break;

        case EMACommand::Follow:
            if (Params.FollowTarget.IsValid())
            {
                if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Agent))
                {
                    Robot->SetFollowTarget(Params.FollowTarget.Get());
                }
                else if (AMADroneCharacter* Drone = Cast<AMADroneCharacter>(Agent))
                {
                    Drone->SetFollowTarget(Params.FollowTarget.Get());
                }
            }
            break;

        case EMACommand::Coverage:
            if (Params.CoverageArea.IsValid())
            {
                if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Agent))
                {
                    Robot->SetCoverageArea(Params.CoverageArea.Get());
                }
                // Drone 暂不支持 CoverageArea
            }
            break;

        default:
            break;
    }
}

FGameplayTag UMACommandManager::CommandToTag(EMACommand Command) const
{
    if (const FGameplayTag* Tag = CommandTagCache.Find(Command))
    {
        return *Tag;
    }
    return FGameplayTag();
}

bool UMACommandManager::ShouldExcludeAgent(AMACharacter* Agent, const FMACommandParams& Params) const
{
    if (!Agent)
    {
        return true;
    }

    if (Params.bExcludeTracker && Agent->AgentName.Contains(TEXT("Tracker")))
    {
        return true;
    }

    return false;
}

// ========== 自动参数填充 ==========

void UMACommandManager::AutoFillCommandParams(EMACommand Command, FMACommandParams& Params)
{
    UWorld* World = GetWorld();
    if (!World) return;

    switch (Command)
    {
        case EMACommand::Patrol:
            // 自动查找 PatrolPath
            if (!Params.PatrolPath.IsValid())
            {
                TArray<AActor*> PatrolPaths;
                UGameplayStatics::GetAllActorsOfClass(World, AMAPatrolPath::StaticClass(), PatrolPaths);
                if (PatrolPaths.Num() > 0)
                {
                    Params.PatrolPath = Cast<AMAPatrolPath>(PatrolPaths[0]);
                }
                else if (Params.bShowMessage)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No PatrolPath in scene!"));
                }
            }
            break;

        case EMACommand::Coverage:
            // 自动查找 CoverageArea
            if (!Params.CoverageArea.IsValid())
            {
                TArray<AActor*> CoverageAreas;
                UGameplayStatics::GetAllActorsOfClass(World, AMACoverageArea::StaticClass(), CoverageAreas);
                if (CoverageAreas.Num() > 0)
                {
                    Params.CoverageArea = CoverageAreas[0];
                }
                else if (Params.bShowMessage)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No CoverageArea in scene!"));
                }
            }
            break;

        case EMACommand::Follow:
            // 自动查找第一个 Human 作为跟随目标
            if (!Params.FollowTarget.IsValid())
            {
                UMAAgentManager* AgentManager = World->GetSubsystem<UMAAgentManager>();
                if (AgentManager)
                {
                    TArray<AMACharacter*> Humans = AgentManager->GetAgentsByType(EMAAgentType::Human);
                    if (Humans.Num() > 0)
                    {
                        Params.FollowTarget = Humans[0];
                    }
                    else if (Params.bShowMessage)
                    {
                        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No Human to follow!"));
                    }
                }
            }
            break;

        default:
            break;
    }
}
