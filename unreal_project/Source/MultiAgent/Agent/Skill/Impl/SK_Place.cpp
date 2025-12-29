// SK_Place.cpp

#include "SK_Place.h"
#include "../MASkillTags.h"
#include "../MASkillComponent.h"
#include "../../Character/MACharacter.h"
#include "../../../Environment/MAPickupItem.h"
#include "TimerManager.h"

USK_Place::USK_Place()
{
    bPlaceSucceeded = false;
    PlaceResultMessage = TEXT("");
}

void USK_Place::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    
    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    CurrentPhase = EPlacePhase::MoveToObject;
    
    // 重置结果状态
    bPlaceSucceeded = false;
    PlaceResultMessage = TEXT("");
    
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: Character not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: SkillComponent not found");
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    const FMASearchResults& Results = SkillComp->GetSearchResults();
    if (!Results.Object1Actor.IsValid())
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: Target object not found");
        Character->ShowStatus(TEXT("[Place] Object not found"), 2.f);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    
    TargetObject = Results.Object1Actor;
    TargetLocation = Results.Object1Location;
    DropLocation = Results.Object2Location;
    
    float DistanceToObject = FVector::Dist(Character->GetActorLocation(), TargetLocation);
    if (DistanceToObject <= InteractionRadius)
    {
        CurrentPhase = EPlacePhase::PickUp;
    }
    
    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Starting..."));
    UpdatePhase();
}

void USK_Place::UpdatePhase()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: Character lost during operation");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        bPlaceSucceeded = false;
        PlaceResultMessage = TEXT("Place failed: SkillComponent lost during operation");
        EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
        return;
    }
    
    switch (CurrentPhase)
    {
        case EPlacePhase::MoveToObject:
        {
            Character->ShowAbilityStatus(TEXT("Place"), TEXT("Moving to object..."));
            if (SkillComp->TryActivateNavigate(TargetLocation))
            {
                if (UWorld* World = Character->GetWorld())
                {
                    World->GetTimerManager().SetTimer(PhaseTimerHandle, [this, Character]()
                    {
                        float Distance = FVector::Dist(Character->GetActorLocation(), TargetLocation);
                        if (Distance <= InteractionRadius || !Character->bIsMoving)
                        {
                            Character->GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
                            CurrentPhase = EPlacePhase::PickUp;
                            UpdatePhase();
                        }
                    }, 0.2f, true);
                }
            }
            else
            {
                bPlaceSucceeded = false;
                PlaceResultMessage = TEXT("Place failed: Could not navigate to object");
                EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
            }
            break;
        }
        
        case EPlacePhase::PickUp:
        {
            Character->ShowAbilityStatus(TEXT("Place"), TEXT("Picking up..."));
            PerformPickup();
            if (UWorld* World = Character->GetWorld())
            {
                World->GetTimerManager().SetTimer(PhaseTimerHandle, [this]()
                {
                    CurrentPhase = EPlacePhase::MoveToTarget;
                    UpdatePhase();
                }, BendDuration, false);
            }
            break;
        }
        
        case EPlacePhase::MoveToTarget:
        {
            Character->ShowAbilityStatus(TEXT("Place"), TEXT("Moving to target..."));
            float DistanceToTarget = FVector::Dist(Character->GetActorLocation(), DropLocation);
            if (DistanceToTarget <= InteractionRadius)
            {
                CurrentPhase = EPlacePhase::PutDown;
                UpdatePhase();
                return;
            }
            if (SkillComp->TryActivateNavigate(DropLocation))
            {
                if (UWorld* World = Character->GetWorld())
                {
                    World->GetTimerManager().SetTimer(PhaseTimerHandle, [this, Character]()
                    {
                        float Distance = FVector::Dist(Character->GetActorLocation(), DropLocation);
                        if (Distance <= InteractionRadius || !Character->bIsMoving)
                        {
                            Character->GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
                            CurrentPhase = EPlacePhase::PutDown;
                            UpdatePhase();
                        }
                    }, 0.2f, true);
                }
            }
            else
            {
                bPlaceSucceeded = false;
                PlaceResultMessage = TEXT("Place failed: Could not navigate to target location");
                EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
            }
            break;
        }
        
        case EPlacePhase::PutDown:
        {
            Character->ShowAbilityStatus(TEXT("Place"), TEXT("Putting down..."));
            PerformPutDown();
            if (UWorld* World = Character->GetWorld())
            {
                World->GetTimerManager().SetTimer(PhaseTimerHandle, [this]()
                {
                    CurrentPhase = EPlacePhase::Complete;
                    UpdatePhase();
                }, BendDuration, false);
            }
            break;
        }
        
        case EPlacePhase::Complete:
        {
            const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
            bPlaceSucceeded = true;
            PlaceResultMessage = FString::Printf(TEXT("Place succeeded: Moved %s to %s at (%.0f, %.0f, %.0f)"), 
                *Context.PlacedObjectName, *Context.PlaceTargetName,
                DropLocation.X, DropLocation.Y, DropLocation.Z);
            Character->ShowAbilityStatus(TEXT("Place"), TEXT("Complete!"));
            EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
            break;
        }
    }
}

void USK_Place::PerformPickup()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !TargetObject.IsValid()) return;
    
    AMAPickupItem* Item = Cast<AMAPickupItem>(TargetObject.Get());
    if (Item)
    {
        Item->OnPickedUp(Character);
        Item->AttachToActor(Character, FAttachmentTransformRules::KeepWorldTransform);
        Item->SetActorRelativeLocation(FVector(0, 0, 100));
        HeldObject = Item;
    }
}

void USK_Place::PerformPutDown()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !HeldObject.IsValid()) return;
    
    AMAPickupItem* Item = Cast<AMAPickupItem>(HeldObject.Get());
    if (Item)
    {
        Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        FVector PlaceLocation = DropLocation;
        PlaceLocation.Z = Character->GetActorLocation().Z;
        Item->OnDropped(PlaceLocation);
        HeldObject.Reset();
    }
}

void USK_Place::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();
    
    // 保存通知所需的信息
    bool bShouldNotify = false;
    bool bSuccessToNotify = bPlaceSucceeded;
    FString MessageToNotify = PlaceResultMessage;
    UMASkillComponent* SkillCompToNotify = nullptr;
    
    if (Character)
    {
        if (UWorld* World = Character->GetWorld())
        {
            World->GetTimerManager().ClearTimer(PhaseTimerHandle);
        }
        PhaseTimerHandle.Invalidate();
        
        Character->ShowStatus(TEXT(""), 0.f);
        if (bWasCancelled && HeldObject.IsValid())
        {
            PerformPutDown();
        }
        
        // 如果被取消且没有设置结果消息，说明是外部取消
        if (bWasCancelled && PlaceResultMessage.IsEmpty())
        {
            bSuccessToNotify = false;
            FString PhaseStr;
            switch (CurrentPhase)
            {
                case EPlacePhase::MoveToObject: PhaseStr = TEXT("moving to object"); break;
                case EPlacePhase::PickUp: PhaseStr = TEXT("picking up"); break;
                case EPlacePhase::MoveToTarget: PhaseStr = TEXT("moving to target"); break;
                case EPlacePhase::PutDown: PhaseStr = TEXT("putting down"); break;
                default: PhaseStr = TEXT("unknown phase"); break;
            }
            MessageToNotify = FString::Printf(TEXT("Place cancelled: Stopped while %s"), *PhaseStr);
        }
        
        // 检查是否需要通知完成
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FGameplayTag PlaceTag = FGameplayTag::RequestGameplayTag(FName("Command.Place"));
            if (SkillComp->HasMatchingGameplayTag(PlaceTag))
            {
                SkillComp->RemoveLooseGameplayTag(PlaceTag);
                bShouldNotify = true;
                SkillCompToNotify = SkillComp;
            }
        }
    }
    
    TargetObject.Reset();
    HeldObject.Reset();
    
    // 先调用父类 EndAbility
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    
    // 在技能完全结束后再通知完成
    if (bShouldNotify && SkillCompToNotify)
    {
        SkillCompToNotify->NotifySkillCompleted(bSuccessToNotify, MessageToNotify);
    }
}
