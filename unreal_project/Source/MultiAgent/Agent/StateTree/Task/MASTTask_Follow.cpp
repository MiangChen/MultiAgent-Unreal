// MASTTask_Follow.cpp

#include "MASTTask_Follow.h"
#include "MASTTaskUtils.h"
#include "../../Character/MACharacter.h"
#include "../../GAS/MAAbilitySystemComponent.h"
#include "../../Interface/MAAgentInterfaces.h"
#include "../../Component/Capability/MACapabilityComponents.h"
#include "StateTreeExecutionContext.h"
#include "GameplayTagContainer.h"

using namespace MASTTaskUtils;

EStateTreeRunStatus FMASTTask_Follow::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);
    Data.TargetCharacter.Reset();
    Data.bAbilityActivated = false;

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] No owner actor"));
        return EStateTreeRunStatus::Failed;
    }

    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] Owner is not AMACharacter"));
        return EStateTreeRunStatus::Failed;
    }

    // 使用 Interface 获取 FollowTarget (支持 Component 模式)
    IMAFollowable* Followable = GetCapabilityInterface<IMAFollowable>(Owner);
    if (!Followable)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] Owner does not have IMAFollowable capability"));
        return EStateTreeRunStatus::Failed;
    }

    AMACharacter* TargetCharacter = Followable->GetFollowTarget();
    if (!TargetCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] %s: No FollowTarget set, cannot follow"),
            *Character->AgentName);
        return EStateTreeRunStatus::Failed;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[STTask_Follow] %s: Following target: %s"),
        *Character->AgentName, *TargetCharacter->AgentName);

    Data.TargetCharacter = TargetCharacter;

    // 激活 Follow ability
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] %s: No ASC found"), *Character->AgentName);
        return EStateTreeRunStatus::Failed;
    }
    
    if (!ASC->TryActivateFollow(TargetCharacter))
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] %s: GA_Follow activation failed"), *Character->AgentName);
        return EStateTreeRunStatus::Failed;
    }
    
    Data.bAbilityActivated = true;
    UE_LOG(LogTemp, Log, TEXT("[STTask_Follow] %s: GA_Follow activated for %s"),
        *Character->AgentName, *TargetCharacter->AgentName);

    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMASTTask_Follow::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMACharacter* Character = Cast<AMACharacter>(Owner);
    if (!Character)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 检查 Command Tag
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Character->GetAbilitySystemComponent());
    if (ASC && !ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Command.Follow"))))
    {
        // 命令被取消
        return EStateTreeRunStatus::Succeeded;
    }

    // 检查目标是否还有效
    if (!Data.TargetCharacter.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] Target lost"));
        return EStateTreeRunStatus::Failed;
    }

    // Follow ability 会自己处理跟随逻辑，这里只需要保持 Running
    return EStateTreeRunStatus::Running;
}

void FMASTTask_Follow::ExitState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (Owner)
    {
        UMAAbilitySystemComponent* ASC = Owner->FindComponentByClass<UMAAbilitySystemComponent>();
        if (ASC)
        {
            if (Data.bAbilityActivated)
            {
                ASC->CancelFollow();
            }
            
            // 如果没有其他命令，设置 Idle 命令
            FGameplayTag IdleTag = FGameplayTag::RequestGameplayTag(FName("Command.Idle"));
            FGameplayTag FollowTag = FGameplayTag::RequestGameplayTag(FName("Command.Follow"));
            FGameplayTag ChargeTag = FGameplayTag::RequestGameplayTag(FName("Command.Charge"));
            FGameplayTag CoverageTag = FGameplayTag::RequestGameplayTag(FName("Command.Coverage"));
            FGameplayTag PatrolTag = FGameplayTag::RequestGameplayTag(FName("Command.Patrol"));
            
            bool bHasOtherCommand = ASC->HasMatchingGameplayTag(FollowTag) ||
                                    ASC->HasMatchingGameplayTag(ChargeTag) ||
                                    ASC->HasMatchingGameplayTag(CoverageTag) ||
                                    ASC->HasMatchingGameplayTag(PatrolTag);
            
            if (!bHasOtherCommand && !ASC->HasMatchingGameplayTag(IdleTag))
            {
                ASC->AddLooseGameplayTag(IdleTag);
                UE_LOG(LogTemp, Log, TEXT("[STTask_Follow] Added Command.Idle for state transition"));
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[STTask_Follow] Follow ended"));
}