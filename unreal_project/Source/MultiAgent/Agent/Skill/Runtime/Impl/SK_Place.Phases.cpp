#include "SK_Place.h"

#include "Agent/Skill/Infrastructure/MASkillPlaceContextBuilder.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAHumanoidCharacter.h"

void USK_Place::UpdatePhase()
{
    switch (CurrentPhase)
    {
        case EPlacePhase::MoveToSource:
            HandleMoveToSource();
            break;
        case EPlacePhase::BendDownPickup:
            HandleBendDownPickup();
            break;
        case EPlacePhase::StandUpWithItem:
            HandleStandUpWithItem();
            break;
        case EPlacePhase::MoveToTarget:
            HandleMoveToTarget();
            break;
        case EPlacePhase::BendDownPlace:
            HandleBendDownPlace();
            break;
        case EPlacePhase::StandUpEmpty:
            HandleStandUpEmpty();
            break;
        case EPlacePhase::Complete:
            HandleComplete();
            break;
    }
}

void USK_Place::HandleMoveToSource()
{
    StartPhaseNavigation(
        SourceLocation,
        TEXT("Moving to source..."),
        TEXT("Could not navigate to source object"),
        true);
}

void USK_Place::OnNavigationToSourceCompleted(bool bSuccess, const FString& Message)
{
    HandlePhaseNavigationCompleted(bSuccess, Message, EPlacePhase::BendDownPickup, true);
}

void USK_Place::HandleBendDownPickup()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Picking up..."));

    if (CurrentMode == EPlaceMode::UnloadToGround)
    {
        PerformPickupFromUGV();
        AdvanceToPhase(EPlacePhase::MoveToTarget);
        return;
    }

    if (!TryPlayBendAnimation())
    {
        PerformPickup();
        AdvanceToPhase(EPlacePhase::StandUpWithItem);
    }
}

void USK_Place::OnBendDownComplete()
{
    if (AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter())
    {
        Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnBendDownComplete);
    }

    HandleBendCompletionTransition();
}

void USK_Place::HandleStandUpWithItem()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Standing up..."));

    if (!TryPlayStandUpAnimation())
    {
        AdvanceToPhase(EPlacePhase::MoveToTarget);
    }
}

void USK_Place::OnStandUpComplete()
{
    if (AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter())
    {
        Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnStandUpComplete);
    }

    HandleStandUpTransition();
}

void USK_Place::HandleMoveToTarget()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailPlace(TEXT("Place failed: Character lost during operation"), TEXT("Character lost during operation"));
        return;
    }

    const FVector CurrentTargetLocation = ResolveCurrentTargetLocation();
    const float DistanceToTarget = FVector::Dist(Character->GetActorLocation(), CurrentTargetLocation);
    if (DistanceToTarget <= InteractionRadius)
    {
        AdvanceToPhase(EPlacePhase::BendDownPlace);
        return;
    }

    StartPhaseNavigation(
        CurrentTargetLocation,
        TEXT("Moving to target..."),
        TEXT("Could not navigate to target location"),
        false);
}

void USK_Place::OnNavigationToTargetCompleted(bool bSuccess, const FString& Message)
{
    HandlePhaseNavigationCompleted(bSuccess, Message, EPlacePhase::BendDownPlace, false);
}

void USK_Place::HandleBendDownPlace()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Placing..."));

    if (CurrentMode == EPlaceMode::LoadToUGV)
    {
        PerformPlaceOnUGV();
        AdvanceToPhase(EPlacePhase::Complete);
        return;
    }

    if (CurrentMode == EPlaceMode::StackOnObject)
    {
        PerformPlaceOnObject();
        AdvanceToPhase(EPlacePhase::Complete);
        return;
    }

    if (!TryPlayBendAnimation())
    {
        PerformPlaceOnGround();
        AdvanceToPhase(EPlacePhase::Complete);
    }
}

void USK_Place::HandleStandUpEmpty()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Standing up..."));

    if (!TryPlayStandUpAnimation())
    {
        AdvanceToPhase(EPlacePhase::Complete);
    }
}

void USK_Place::HandleComplete()
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character) return;

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp) return;

    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    bPlaceSucceeded = true;

    const FMAPlaceContextConfig Config{
        CurrentMode,
        SourceObject,
        TargetObject,
        TargetUGV,
        SourceLocation,
        TargetLocation,
        DropLocation};
    const FMAPlaceCompletionFeedback Completion = FMASkillPlaceContextBuilder::BuildCompletion(Context, Config);
    Context.PlaceTargetName = Completion.TargetName;
    Context.PlaceFinalLocation = Completion.FinalLocation;

    PlaceResultMessage = FString::Printf(TEXT("Place succeeded: Moved %s to %s"),
        *Context.PlacedObjectName, *Completion.TargetName);

    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Complete!"));
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, false);
}
