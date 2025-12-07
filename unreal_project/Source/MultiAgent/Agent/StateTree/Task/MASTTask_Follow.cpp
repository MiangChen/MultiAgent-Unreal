// MASTTask_Follow.cpp

#include "MASTTask_Follow.h"
#include "MARobotDogCharacter.h"
#include "MACharacter.h"
#include "MAAbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GameplayTagContainer.h"

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

    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    if (!Robot)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] Owner is not RobotDog"));
        return EStateTreeRunStatus::Failed;
    }

    // 必须有预设的 FollowTarget
    AMACharacter* TargetCharacter = Robot->GetFollowTarget();
    
    if (!TargetCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] %s: No FollowTarget set, cannot follow"),
            *Robot->ActorName);
        return EStateTreeRunStatus::Failed;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[STTask_Follow] %s: Following target: %s"),
        *Robot->ActorName, *TargetCharacter->ActorName);

    Data.TargetCharacter = TargetCharacter;

    // 激活 Follow ability
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Robot->GetAbilitySystemComponent());
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] %s: No ASC found"), *Robot->ActorName);
        return EStateTreeRunStatus::Failed;
    }
    
    if (!ASC->TryActivateFollow(TargetCharacter))
    {
        UE_LOG(LogTemp, Warning, TEXT("[STTask_Follow] %s: GA_Follow activation failed"), *Robot->ActorName);
        return EStateTreeRunStatus::Failed;
    }
    
    Data.bAbilityActivated = true;
    UE_LOG(LogTemp, Log, TEXT("[STTask_Follow] %s: GA_Follow activated for %s"),
        *Robot->ActorName, *TargetCharacter->ActorName);

    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMASTTask_Follow::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FMASTTask_FollowInstanceData& Data = Context.GetInstanceData<FMASTTask_FollowInstanceData>(*this);

    AActor* Owner = Cast<AActor>(Context.GetOwner());
    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Owner);
    if (!Robot)
    {
        return EStateTreeRunStatus::Failed;
    }

    // 检查 Command Tag
    UMAAbilitySystemComponent* ASC = Cast<UMAAbilitySystemComponent>(Robot->GetAbilitySystemComponent());
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
            
            // 如果没有其他命令，设置 Idle 命令以便 StateTree 能正确转换
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
