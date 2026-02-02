// SK_Charge.cpp

#include "SK_Charge.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Environment/Entity/MAChargingStation.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

USK_Charge::USK_Charge()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Charging);
    bChargeSucceeded = false;
    ChargeResultMessage = TEXT("");
}

AMAChargingStation* USK_Charge::FindNearbyChargingStation() const
{
    AMACharacter* Character = const_cast<USK_Charge*>(this)->GetOwningCharacter();
    if (!Character) return nullptr;
    
    UWorld* World = Character->GetWorld();
    if (!World) return nullptr;
    
    TArray<AActor*> FoundStations;
    UGameplayStatics::GetAllActorsOfClass(World, AMAChargingStation::StaticClass(), FoundStations);
    
    for (AActor* Actor : FoundStations)
    {
        AMAChargingStation* Station = Cast<AMAChargingStation>(Actor);
        if (Station && Station->IsRobotInRange(Character))
        {
            return Station;
        }
    }
    return nullptr;
}

bool USK_Charge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
    return FindNearbyChargingStation() != nullptr;
}

void USK_Charge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    CurrentChargingStation = FindNearbyChargingStation();
    
    // 重置结果状态
    bChargeSucceeded = false;
    ChargeResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->ShowStatus(TEXT("[Charging...]"), 9999.f);
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().SetTimer(ChargeTimerHandle, this, &USK_Charge::UpdateCharge, ChargeUpdateInterval, true);
        }
    }
}

void USK_Charge::UpdateCharge()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bChargeSucceeded = false;
        ChargeResultMessage = TEXT("Charge failed: Character lost during charging");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bChargeSucceeded = false;
        ChargeResultMessage = TEXT("Charge failed: SkillComponent lost during charging");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    if (!CurrentChargingStation.IsValid() || !CurrentChargingStation->IsRobotInRange(Character))
    {
        bChargeSucceeded = false;
        float Percent = SkillComp->GetEnergyPercent();
        ChargeResultMessage = FString::Printf(TEXT("Charge interrupted: Left charging station, energy: %.0f%%"), Percent);
        Character->ShowStatus(TEXT("[Charge interrupted]"), 2.f);
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    float ChargeAmount = (ChargeRatePerSecond / 100.f) * SkillComp->MaxEnergy * ChargeUpdateInterval;
    SkillComp->RestoreEnergy(ChargeAmount);
    
    float Percent = SkillComp->GetEnergyPercent();
    Character->ShowStatus(FString::Printf(TEXT("[Charging] %.0f%%"), Percent), 9999.f);
    
    if (Percent >= 100.f)
    {
        bChargeSucceeded = true;
        ChargeResultMessage = FString::Printf(TEXT("Charge succeeded: Fully charged to %.0f%%"), Percent);
        Character->ShowAbilityStatus(TEXT("Charge"), TEXT("Complete!"));
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
    }
}

void USK_Charge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bChargeSucceeded;
    FString MessageToNotify = ChargeResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(ChargeTimerHandle);
        }
        ChargeTimerHandle.Invalidate();
        
        Character->ShowStatus(TEXT(""), 0.f);
        
        // 如果被取消且没有设置结果消息，说明是外部取消
        if (bWasCancelled && ChargeResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
            {
                float Percent = SkillComp->GetEnergyPercent();
                MessageToNotify = FString::Printf(TEXT("Charge cancelled: Stopped at %.0f%%"), Percent);
            }
            else
            {
                MessageToNotify = TEXT("Charge cancelled: Stopped by user");
            }
        }
        
        // 检查是否需要通知完成
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag ChargeTag = FGameplayTag::RequestGameplayTag(FName("Command.Charge"));
            if (SkillComp->HasMatchingGameplayTag(ChargeTag))
            {
                SkillComp->RemoveLooseGameplayTag(ChargeTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    CurrentChargingStation.Reset();
    
    // 先调用父类 EndAbility
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    // 在技能完全结束后再通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
