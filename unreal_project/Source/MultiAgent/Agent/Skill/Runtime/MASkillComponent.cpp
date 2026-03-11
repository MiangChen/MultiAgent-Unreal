// MASkillComponent.cpp
// ========== 技能管理层实现 ==========

#include "MASkillComponent.h"
#include "Agent/Skill/Bootstrap/MASkillBootstrap.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"

UMASkillComponent::UMASkillComponent()
{
}

void UMASkillComponent::InitializeSkills(AActor* InOwnerActor)
{
    FMASkillBootstrap::GrantDefaultAbilities(*this, InOwnerActor);
}

// ========== 命令系统 ==========

void UMASkillComponent::ClearAllCommands()
{
    FGameplayTagContainer OwnedTags;
    GetOwnedGameplayTags(OwnedTags);
    
    for (const FGameplayTag& Tag : OwnedTags)
    {
        if (Tag.ToString().StartsWith(TEXT("Command.")))
        {
            RemoveLooseGameplayTag(Tag);
        }
    }
}

void UMASkillComponent::CancelAllSkills()
{
    // 先清除所有命令 Tag（防止 EndAbility 触发错误通知）
    ClearAllCommands();
    
    // 取消所有技能
    if (NavigateSkillHandle.IsValid()) CancelAbilityHandle(NavigateSkillHandle);
    if (FollowSkillHandle.IsValid()) CancelAbilityHandle(FollowSkillHandle);
    if (ChargeSkillHandle.IsValid()) CancelAbilityHandle(ChargeSkillHandle);
    if (SearchSkillHandle.IsValid()) CancelAbilityHandle(SearchSkillHandle);
    if (PlaceSkillHandle.IsValid()) CancelAbilityHandle(PlaceSkillHandle);
    if (TakeOffSkillHandle.IsValid()) CancelAbilityHandle(TakeOffSkillHandle);
    if (LandSkillHandle.IsValid()) CancelAbilityHandle(LandSkillHandle);
    if (ReturnHomeSkillHandle.IsValid()) CancelAbilityHandle(ReturnHomeSkillHandle);
    if (TakePhotoSkillHandle.IsValid()) CancelAbilityHandle(TakePhotoSkillHandle);
    if (BroadcastSkillHandle.IsValid()) CancelAbilityHandle(BroadcastSkillHandle);
    if (HandleHazardSkillHandle.IsValid()) CancelAbilityHandle(HandleHazardSkillHandle);
    if (GuideSkillHandle.IsValid()) CancelAbilityHandle(GuideSkillHandle);
}

// ========== 反馈上下文 ==========

void UMASkillComponent::UpdateFeedback(int32 Current, int32 Total)
{
    FeedbackContext.CurrentStep = Current;
    FeedbackContext.TotalSteps = Total;
}

void UMASkillComponent::AddFoundObject(const FString& ObjectName, const FVector& Location, const TMap<FString, FString>& Attributes)
{
    FeedbackContext.FoundObjects.Add(ObjectName);
    FeedbackContext.FoundLocations.Add(Location);
    
    for (const auto& Pair : Attributes)
    {
        FeedbackContext.ObjectAttributes.Add(
            FString::Printf(TEXT("%s_%s"), *ObjectName, *Pair.Key), 
            Pair.Value);
    }
}

void UMASkillComponent::PrepareNavigateActivation(const FVector TargetLocation)
{
    SkillParams.TargetLocation = TargetLocation;
}

void UMASkillComponent::PrepareFollowActivation(AActor* TargetActor, const float FollowDistance)
{
    SkillRuntimeTargets.FollowTarget = TargetActor;
    SkillParams.FollowDistance = FollowDistance;

    const FString TargetName = TargetActor ? TargetActor->GetName() : FString();
    FeedbackContext.TargetName = TargetName;
    FeedbackContext.FollowTargetName = TargetName;
    FeedbackContext.bFollowTargetFound = TargetActor != nullptr;
    FeedbackContext.TargetLocation = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
}

void UMASkillComponent::PrepareChargeActivation()
{
    FeedbackContext.EnergyBefore = GetEnergyPercent();
}

bool UMASkillComponent::TryActivateSkillHandle(const FGameplayAbilitySpecHandle AbilityHandle, const TCHAR* SkillName)
{
    if (!AbilityHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TryActivate%s] Ability handle is invalid"), SkillName);
        return false;
    }

    CancelAbilityHandle(AbilityHandle);
    return TryActivateAbility(AbilityHandle);
}

void UMASkillComponent::CancelSkillHandleIfValid(const FGameplayAbilitySpecHandle AbilityHandle)
{
    if (AbilityHandle.IsValid())
    {
        CancelAbilityHandle(AbilityHandle);
    }
}

// ========== 能量系统 ==========

void UMASkillComponent::DrainEnergy(float DeltaTime)
{
    if (Energy <= 0.f) return;
    SetEnergy(FMath::Max(0.f, Energy - EnergyDrainRate * DeltaTime));
}

void UMASkillComponent::SetEnergy(float NewEnergy)
{
    float OldPercent = GetEnergyPercent();
    Energy = FMath::Clamp(NewEnergy, 0.f, MaxEnergy);
    float NewPercent = GetEnergyPercent();
    
    // 跨越低电量阈值时触发一次
    if (OldPercent >= LowEnergyThreshold && NewPercent < LowEnergyThreshold)
    {
        OnLowEnergy.Broadcast();
    }
    
    // 电量归零时触发
    if (OldPercent > 0.f && NewPercent <= 0.f)
    {
        OnEnergyDepleted.Broadcast();
    }
}

void UMASkillComponent::RestoreEnergy(float Amount)
{
    Energy = FMath::Min(MaxEnergy, Energy + Amount);
    FeedbackContext.EnergyAfter = GetEnergyPercent();
}

// ========== 技能完成通知 ==========

void UMASkillComponent::NotifySkillCompleted(bool bSuccess, const FString& Message)
{
    AMACharacter* OwnerCharacter = Cast<AMACharacter>(GetOwner());
    OnSkillCompleted.Broadcast(OwnerCharacter, bSuccess, Message);
}
