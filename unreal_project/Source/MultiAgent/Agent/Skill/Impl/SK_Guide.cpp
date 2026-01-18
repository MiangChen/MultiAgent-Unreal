// SK_Guide.cpp

#include "SK_Guide.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "Containers/Ticker.h"
#include "GameFramework/CharacterMovementComponent.h"

USK_Guide::USK_Guide()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Moving);
}

void USK_Guide::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 获取引导参数
    if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
    {
        const FMASkillParams& Params = SkillComp->GetSkillParams();
        GuideTargetActor = Params.GuideTargetActor;
        GuideDestination = Params.GuideDestination;
    }

    // 验证参数
    if (!GuideTargetActor.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Guide] No valid target actor to guide"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (GuideDestination.IsZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Guide] No valid destination"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 初始化状态
    StartTime = Character->GetWorld()->GetTimeSeconds();
    Character->bIsMoving = true;

    // 显示引导状态
    Character->ShowAbilityStatus(
        TEXT("Guide"),
        FString::Printf(TEXT("Guiding to (%.0f, %.0f, %.0f)"), 
            GuideDestination.X, GuideDestination.Y, GuideDestination.Z)
    );

    // 启动 Ticker 更新引导逻辑
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &USK_Guide::TickGuide),
        0.0f
    );
}

bool USK_Guide::TickGuide(float DeltaTime)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        OnGuideComplete();
        return false;  // 停止 Ticker
    }

    // 检查目标是否仍然有效
    if (!GuideTargetActor.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Guide] Target actor lost during guide"));
        OnGuideComplete();
        return false;
    }

    FVector CurrentLocation = Character->GetActorLocation();
    float Distance2D = FVector::Dist2D(CurrentLocation, GuideDestination);

    // 检查是否到达目的地
    if (Distance2D < AcceptanceRadius)
    {
        OnGuideComplete();
        return false;  // 停止 Ticker
    }

    // Agent 向目的地移动
    FVector DirectionToDestination = (GuideDestination - CurrentLocation).GetSafeNormal2D();
    Character->AddMovementInput(DirectionToDestination, 1.0f);

    // 更新被引导对象的位置（跟随 Agent）
    UpdateTargetPosition(DeltaTime);

    return true;  // 继续 Ticker
}

void USK_Guide::UpdateTargetPosition(float DeltaTime)
{
    if (!GuideTargetActor.IsValid()) return;

    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    // 计算目标应该跟随的位置（在 Agent 后方一定距离）
    FVector AgentLocation = Character->GetActorLocation();
    FVector AgentForward = Character->GetActorForwardVector();
    FVector TargetFollowPosition = AgentLocation - AgentForward * FollowDistance;

    // 获取目标当前位置
    FVector TargetLocation = GuideTargetActor->GetActorLocation();
    float DistanceToFollowPos = FVector::Dist2D(TargetLocation, TargetFollowPosition);

    // 如果距离超过阈值，移动目标
    if (DistanceToFollowPos > 50.f)
    {
        FVector MoveDirection = (TargetFollowPosition - TargetLocation).GetSafeNormal2D();
        
        // 计算移动速度（基于距离，距离越远速度越快）
        float MoveSpeed = FMath::Clamp(DistanceToFollowPos * 2.f, 100.f, 400.f);
        FVector NewLocation = TargetLocation + MoveDirection * MoveSpeed * DeltaTime;
        
        // 保持 Z 轴不变（或者可以根据地形调整）
        NewLocation.Z = TargetLocation.Z;
        
        GuideTargetActor->SetActorLocation(NewLocation);
    }
}

void USK_Guide::OnGuideComplete()
{
    AMACharacter* Character = GetOwningCharacter();
    
    if (Character)
    {
        Character->bIsMoving = false;
        Character->ShowStatus(TEXT(""), 0.f);
    }

    // 通知技能完成
    bool bShouldNotify = false;
    bool bSuccessToNotify = true;
    FString MessageToNotify;
    UMASkillComponent* SkillCompToNotify = nullptr;

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag GuideTag = FGameplayTag::RequestGameplayTag(FName("Command.Guide"));
            if (SkillComp->HasMatchingGameplayTag(GuideTag))
            {
                SkillComp->RemoveLooseGameplayTag(GuideTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;

                // 计算引导持续时间
                float CurrentTime = Character->GetWorld()->GetTimeSeconds();
                float Duration = CurrentTime - StartTime;

                // 记录到反馈上下文
                FMAFeedbackContext& FeedbackCtx = SkillComp->GetFeedbackContextMutable();
                FeedbackCtx.GuideDurationSeconds = Duration;

                // 生成消息
                MessageToNotify = FString::Printf(
                    TEXT("Guide succeeded, target guided to (%.0f, %.0f, %.0f)"),
                    GuideDestination.X, GuideDestination.Y, GuideDestination.Z
                );
            }
        }
    }

    // 结束技能
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);

    // 在技能完全结束后通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}

void USK_Guide::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 清理 Ticker
    if (TickDelegateHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
        TickDelegateHandle.Reset();
    }

    // 清理移动状态
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->bIsMoving = false;
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
