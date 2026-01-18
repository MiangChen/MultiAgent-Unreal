// MASkillComponent.cpp
// ========== 技能管理层实现 ==========

#include "MASkillComponent.h"
#include "Impl/SK_Navigate.h"
#include "Impl/SK_Follow.h"
#include "Impl/SK_Charge.h"
#include "Impl/SK_Search.h"
#include "Impl/SK_Place.h"
#include "Impl/SK_TakeOff.h"
#include "Impl/SK_Land.h"
#include "Impl/SK_ReturnHome.h"
#include "Impl/SK_TakePhoto.h"
#include "Impl/SK_Broadcast.h"
#include "Impl/SK_HandleHazard.h"
#include "Impl/SK_Guide.h"
#include "../Character/MACharacter.h"

UMASkillComponent::UMASkillComponent()
{
}

void UMASkillComponent::InitializeSkills(AActor* InOwnerActor)
{
    if (!InOwnerActor) return;

    NavigateSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_Navigate::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    FollowSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_Follow::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    ChargeSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_Charge::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    SearchSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_Search::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    PlaceSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_Place::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    TakeOffSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_TakeOff::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    LandSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_Land::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    ReturnHomeSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_ReturnHome::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    TakePhotoSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_TakePhoto::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    BroadcastSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_Broadcast::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    HandleHazardSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_HandleHazard::StaticClass(), 1, INDEX_NONE, InOwnerActor));
    GuideSkillHandle = GiveAbility(FGameplayAbilitySpec(USK_Guide::StaticClass(), 1, INDEX_NONE, InOwnerActor));
}

// ========== 技能激活 ==========

bool UMASkillComponent::TryActivateNavigate(FVector TargetLocation)
{
    if (!NavigateSkillHandle.IsValid()) return false;
    
    // 先取消当前正在运行的导航技能（如果有）
    CancelAbilityHandle(NavigateSkillHandle);
    
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(NavigateSkillHandle);
    if (Spec)
    {
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (USK_Navigate* NavigateSkill = Cast<USK_Navigate>(Instance))
        {
            NavigateSkill->SetTargetLocation(TargetLocation);
        }
    }
    
    return TryActivateAbility(NavigateSkillHandle);
}

void UMASkillComponent::CancelNavigate()
{
    if (NavigateSkillHandle.IsValid())
    {
        CancelAbilityHandle(NavigateSkillHandle);
    }
}

bool UMASkillComponent::TryActivateFollow(AMACharacter* TargetCharacter, float FollowDistance)
{
    if (!FollowSkillHandle.IsValid() || !TargetCharacter) return false;
    
    // 先取消当前正在运行的跟随技能（如果有）
    CancelAbilityHandle(FollowSkillHandle);
    
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(FollowSkillHandle);
    if (Spec)
    {
        UGameplayAbility* Instance = Spec->GetPrimaryInstance();
        if (!Instance)
        {
            Instance = CreateNewInstanceOfAbility(*Spec, Spec->Ability);
        }
        
        if (USK_Follow* FollowSkill = Cast<USK_Follow>(Instance))
        {
            FollowSkill->SetTargetCharacter(TargetCharacter);
            FollowSkill->SetFollowDistance(FollowDistance);
        }
    }
    
    FeedbackContext.TargetName = TargetCharacter->AgentName;
    
    return TryActivateAbility(FollowSkillHandle);
}

void UMASkillComponent::CancelFollow()
{
    if (FollowSkillHandle.IsValid())
    {
        CancelAbilityHandle(FollowSkillHandle);
    }
}

bool UMASkillComponent::TryActivateCharge()
{
    if (!ChargeSkillHandle.IsValid()) return false;
    
    CancelAbilityHandle(ChargeSkillHandle);
    FeedbackContext.EnergyBefore = GetEnergyPercent();
    return TryActivateAbility(ChargeSkillHandle);
}

bool UMASkillComponent::TryActivateSearch()
{
    if (!SearchSkillHandle.IsValid()) return false;
    
    CancelAbilityHandle(SearchSkillHandle);
    return TryActivateAbility(SearchSkillHandle);
}

void UMASkillComponent::CancelSearch()
{
    if (SearchSkillHandle.IsValid())
    {
        CancelAbilityHandle(SearchSkillHandle);
    }
}

bool UMASkillComponent::TryActivatePlace()
{
    if (!PlaceSkillHandle.IsValid()) return false;
    
    CancelAbilityHandle(PlaceSkillHandle);
    return TryActivateAbility(PlaceSkillHandle);
}

void UMASkillComponent::CancelPlace()
{
    if (PlaceSkillHandle.IsValid())
    {
        CancelAbilityHandle(PlaceSkillHandle);
    }
}

bool UMASkillComponent::TryActivateTakeOff()
{
    if (!TakeOffSkillHandle.IsValid()) return false;
    
    CancelAbilityHandle(TakeOffSkillHandle);
    return TryActivateAbility(TakeOffSkillHandle);
}

bool UMASkillComponent::TryActivateLand()
{
    if (!LandSkillHandle.IsValid()) return false;
    
    CancelAbilityHandle(LandSkillHandle);
    return TryActivateAbility(LandSkillHandle);
}

bool UMASkillComponent::TryActivateReturnHome()
{
    if (!ReturnHomeSkillHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TryActivateReturnHome] ReturnHomeSkillHandle is invalid!"));
        return false;
    }
    
    CancelAbilityHandle(ReturnHomeSkillHandle);
    return TryActivateAbility(ReturnHomeSkillHandle);
}

void UMASkillComponent::CancelReturnHome()
{
    if (ReturnHomeSkillHandle.IsValid())
    {
        CancelAbilityHandle(ReturnHomeSkillHandle);
    }
}

bool UMASkillComponent::TryActivateTakePhoto()
{
    if (!TakePhotoSkillHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TryActivateTakePhoto] TakePhotoSkillHandle is invalid!"));
        return false;
    }
    
    CancelAbilityHandle(TakePhotoSkillHandle);
    return TryActivateAbility(TakePhotoSkillHandle);
}

bool UMASkillComponent::TryActivateBroadcast()
{
    if (!BroadcastSkillHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TryActivateBroadcast] BroadcastSkillHandle is invalid!"));
        return false;
    }
    
    CancelAbilityHandle(BroadcastSkillHandle);
    return TryActivateAbility(BroadcastSkillHandle);
}

bool UMASkillComponent::TryActivateHandleHazard()
{
    if (!HandleHazardSkillHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TryActivateHandleHazard] HandleHazardSkillHandle is invalid!"));
        return false;
    }
    
    CancelAbilityHandle(HandleHazardSkillHandle);
    return TryActivateAbility(HandleHazardSkillHandle);
}

bool UMASkillComponent::TryActivateGuide()
{
    if (!GuideSkillHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TryActivateGuide] GuideSkillHandle is invalid!"));
        return false;
    }
    
    CancelAbilityHandle(GuideSkillHandle);
    return TryActivateAbility(GuideSkillHandle);
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

// ========== 能量系统 ==========

void UMASkillComponent::DrainEnergy(float DeltaTime)
{
    if (Energy <= 0.f) return;
    
    float OldEnergy = Energy;
    Energy = FMath::Max(0.f, Energy - EnergyDrainRate * DeltaTime);
    
    if (OldEnergy > 0.f && Energy <= 0.f)
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
