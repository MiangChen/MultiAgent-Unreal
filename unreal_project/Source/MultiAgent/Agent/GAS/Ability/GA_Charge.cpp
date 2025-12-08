// GA_Charge.cpp

#include "GA_Charge.h"
#include "../MAGameplayTags.h"
#include "MACharacter.h"
#include "MARobotDogCharacter.h"
#include "MAChargingStation.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UGA_Charge::UGA_Charge()
{
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(FMAGameplayTags::Get().Ability_Charge);
    SetAssetTags(AssetTags);
    
    ActivationOwnedTags.AddTag(FMAGameplayTags::Get().Status_Charging);
}

AMAChargingStation* UGA_Charge::FindNearbyChargingStation() const
{
    AMACharacter* Character = const_cast<UGA_Charge*>(this)->GetOwningCharacter();
    if (!Character)
    {
        return nullptr;
    }
    
    UWorld* World = Character->GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    // Find all charging stations
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

bool UGA_Charge::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    
    // Must be a RobotDog
    AMACharacter* Character = const_cast<UGA_Charge*>(this)->GetOwningCharacter();
    if (!Cast<AMARobotDogCharacter>(Character))
    {
        return false;
    }
    
    // Must be in range of a charging station
    return FindNearbyChargingStation() != nullptr;
}

void UGA_Charge::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    
    // 保存当前充电站
    CurrentChargingStation = FindNearbyChargingStation();
    
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        Character->ShowStatus(TEXT("[Charging...]"), 9999.f);
        UE_LOG(LogTemp, Log, TEXT("[GA_Charge] %s started charging"), *Character->AgentName);
        
        // 启动定时器进行渐进式充电
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().SetTimer(
                ChargeTimerHandle,
                this,
                &UGA_Charge::UpdateCharge,
                ChargeUpdateInterval,
                true  // 循环
            );
        }
    }
}

void UGA_Charge::UpdateCharge()
{
    AMACharacter* Character = GetOwningCharacter();
    AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Character);
    
    if (!Robot)
    {
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // 检查是否还在充电站范围内
    if (!CurrentChargingStation.IsValid() || !CurrentChargingStation->IsRobotInRange(Robot))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GA_Charge] %s left charging station range"), *Robot->AgentName);
        Robot->ShowStatus(TEXT("[Charge interrupted]"), 2.f);
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    // 计算本次充电量
    float ChargeAmount = (ChargeRatePerSecond / 100.f) * Robot->MaxEnergy * ChargeUpdateInterval;
    Robot->RestoreEnergy(ChargeAmount);
    
    // 更新显示
    float Percent = Robot->GetEnergyPercent();
    Robot->ShowStatus(FString::Printf(TEXT("[Charging] %.0f%%"), Percent), 9999.f);
    
    // 检查是否充满
    if (Percent >= 100.f)
    {
        UE_LOG(LogTemp, Log, TEXT("[GA_Charge] %s fully charged!"), *Robot->AgentName);
        Robot->ShowAbilityStatus(TEXT("Charge"), TEXT("Complete!"));
        
        // 发送充电完成事件
        if (UAbilitySystemComponent* ASC = Robot->GetAbilitySystemComponent())
        {
            FGameplayEventData EventData;
            EventData.Instigator = Robot;
            ASC->HandleGameplayEvent(FMAGameplayTags::Get().Event_Charge_Complete, &EventData);
        }
        
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
    }
}

void UGA_Charge::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 清理定时器
    AMACharacter* Character = GetOwningCharacter();
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(ChargeTimerHandle);
        }
        Character->ShowStatus(TEXT(""), 0.f);
    }
    
    CurrentChargingStation.Reset();
    
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
