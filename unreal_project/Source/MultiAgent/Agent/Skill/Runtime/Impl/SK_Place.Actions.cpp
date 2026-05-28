#include "SK_Place.h"

#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAHumanoidCharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAUGVCharacter.h"
#include "Environment/IMAPickupItem.h"

void USK_Place::PerformPickup()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !SourceObject.IsValid()) return;

    IMAPickupItem* Item = Cast<IMAPickupItem>(SourceObject.Get());
    if (Item)
    {
        Item->AttachToHand(Character);
        HeldObject = SourceObject;

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlacedObjectName = Item->GetItemName();
        }
    }
}

void USK_Place::PerformPickupFromUGV()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !SourceObject.IsValid()) return;

    IMAPickupItem* Item = Cast<IMAPickupItem>(SourceObject.Get());
    if (!Item) return;

    Item->DetachFromCarrier();
    Item->AttachToHand(Character);
    HeldObject = SourceObject;

    if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
    {
        SkillComp->GetFeedbackContextMutable().PlacedObjectName = Item->GetItemName();
    }
}

void USK_Place::PerformPlaceOnGround()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !HeldObject.IsValid()) return;

    IMAPickupItem* Item = Cast<IMAPickupItem>(HeldObject.Get());
    if (Item)
    {
        FVector PlaceLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 120.f;
        PlaceLocation.Z = Character->GetActorLocation().Z;
        Item->PlaceOnGround(PlaceLocation);
        HeldObject.Reset();

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceTargetName = TEXT("ground");
        }
    }
}

void USK_Place::PerformPlaceOnUGV()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !HeldObject.IsValid() || !TargetUGV.IsValid()) return;

    IMAPickupItem* Item = Cast<IMAPickupItem>(HeldObject.Get());
    if (Item)
    {
        Item->AttachToUGV(TargetUGV.Get());
        HeldObject.Reset();

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceTargetName = TargetUGV->AgentLabel;
        }
    }
}

void USK_Place::PerformPlaceOnObject()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character || !HeldObject.IsValid() || !TargetObject.IsValid()) return;

    IMAPickupItem* Item = Cast<IMAPickupItem>(HeldObject.Get());
    IMAPickupItem* Target = Cast<IMAPickupItem>(TargetObject.Get());

    if (Item && Target)
    {
        Item->PlaceOnObject(TargetObject.Get());
        HeldObject.Reset();

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceTargetName = Target->GetItemName();
        }
    }
}

AMAHumanoidCharacter* USK_Place::GetHumanoidCharacter() const
{
    return Cast<AMAHumanoidCharacter>(GetOwningCharacter());
}

FString USK_Place::GetPhaseString() const
{
    switch (CurrentPhase)
    {
        case EPlacePhase::MoveToSource: return TEXT("moving to source");
        case EPlacePhase::BendDownPickup: return TEXT("bending down to pickup");
        case EPlacePhase::StandUpWithItem: return TEXT("standing up with item");
        case EPlacePhase::MoveToTarget: return TEXT("moving to target");
        case EPlacePhase::BendDownPlace: return TEXT("bending down to place");
        case EPlacePhase::StandUpEmpty: return TEXT("standing up");
        case EPlacePhase::Complete: return TEXT("complete");
        default: return TEXT("unknown phase");
    }
}
