#include "SK_Place.h"

#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "Agent/Skill/Infrastructure/MASkillPlaceContextBuilder.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAHumanoidCharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAUGVCharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Environment/IMAPickupItem.h"
#include "TimerManager.h"

USK_Place::USK_Place()
{
    bPlaceSucceeded = false;
    PlaceResultMessage = TEXT("");
}

void USK_Place::ResetPlaceRuntimeState()
{
    bPlaceSucceeded = false;
    PlaceResultMessage = TEXT("");
    CurrentPhase = EPlacePhase::MoveToSource;
}

void USK_Place::FailPlace(const FString& ResultMessage, const FString& ErrorReason, const FString& StatusMessage)
{
    bPlaceSucceeded = false;
    PlaceResultMessage = ResultMessage;

    if (AMACharacter* Character = GetOwningCharacter())
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceErrorReason = ErrorReason;
        }
        if (!StatusMessage.IsEmpty())
        {
            Character->ShowStatus(StatusMessage, 2.f);
        }
    }

    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, true);
}

bool USK_Place::InitializePlaceContext(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    const FMAPlaceContextBuildFeedback BuildFeedback = FMASkillPlaceContextBuilder::Build(Character, SkillComp);
    if (!BuildFeedback.bSuccess)
    {
        FailPlace(BuildFeedback.FailureResultMessage, BuildFeedback.FailureReason, BuildFeedback.FailureStatusMessage);
        return false;
    }

    ApplyPlaceContextConfig(BuildFeedback.Config);

    if (FVector::Dist(Character.GetActorLocation(), SourceLocation) <= InteractionRadius)
    {
        CurrentPhase = EPlacePhase::BendDownPickup;
    }

    NavigationService = Character.GetNavigationService();
    if (!NavigationService)
    {
        FailPlace(TEXT("Place failed: NavigationService not found"), TEXT("NavigationService not found"));
        return false;
    }

    return true;
}

void USK_Place::ApplyPlaceContextConfig(const FMAPlaceContextConfig& Config)
{
    CurrentMode = Config.Mode;
    SourceObject = Config.SourceObject;
    TargetObject = Config.TargetObject;
    TargetUGV = Config.TargetUGV;
    SourceLocation = Config.SourceLocation;
    TargetLocation = Config.TargetLocation;
    DropLocation = Config.DropLocation;
}

void USK_Place::AdvanceToPhase(const EPlacePhase NextPhase)
{
    CurrentPhase = NextPhase;
    UpdatePhase();
}

void USK_Place::HandleNavigationFailure(const FString& Message)
{
    FailPlace(FString::Printf(TEXT("Place failed: %s"), *Message), Message);
}

bool USK_Place::StartPhaseNavigation(
    const FVector& Destination,
    const TCHAR* StatusText,
    const TCHAR* FailureReason,
    const bool bToSource)
{
    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailPlace(TEXT("Place failed: Character lost during operation"), TEXT("Character lost during operation"));
        return false;
    }

    if (!NavigationService)
    {
        FailPlace(TEXT("Place failed: NavigationService lost during operation"), TEXT("NavigationService lost during operation"));
        return false;
    }

    Character->ShowAbilityStatus(TEXT("Place"), StatusText);

    if (bToSource)
    {
        NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
    }
    else
    {
        NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
    }

    if (NavigationService->NavigateTo(Destination, InteractionRadius))
    {
        return true;
    }

    if (bToSource)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
    }
    else
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
    }

    FailPlace(
        FString::Printf(TEXT("Place failed: %s"), FailureReason),
        FailureReason);
    return false;
}

void USK_Place::HandlePhaseNavigationCompleted(
    const bool bSuccess,
    const FString& Message,
    const EPlacePhase SuccessPhase,
    const bool bToSource)
{
    if (NavigationService)
    {
        if (bToSource)
        {
            NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
        }
        else
        {
            NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
        }
    }

    if (bSuccess)
    {
        AdvanceToPhase(SuccessPhase);
        return;
    }

    HandleNavigationFailure(Message);
}

bool USK_Place::TryPlayBendAnimation()
{
    if (AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter())
    {
        Humanoid->OnBendAnimationComplete.AddDynamic(this, &USK_Place::OnBendDownComplete);
        Humanoid->PlayBendDownAnimation();
        return true;
    }

    return false;
}

bool USK_Place::TryPlayStandUpAnimation()
{
    if (AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter())
    {
        Humanoid->OnBendAnimationComplete.AddDynamic(this, &USK_Place::OnStandUpComplete);
        Humanoid->PlayStandUpAnimation();
        return true;
    }

    return false;
}

void USK_Place::HandleBendCompletionTransition()
{
    if (CurrentPhase == EPlacePhase::BendDownPickup)
    {
        PerformPickup();
        AdvanceToPhase(EPlacePhase::StandUpWithItem);
        return;
    }

    if (CurrentPhase == EPlacePhase::BendDownPlace)
    {
        PerformPlaceOnGround();
        AdvanceToPhase(EPlacePhase::StandUpEmpty);
    }
}

void USK_Place::HandleStandUpTransition()
{
    if (CurrentPhase == EPlacePhase::StandUpWithItem)
    {
        AdvanceToPhase(EPlacePhase::MoveToTarget);
        return;
    }

    if (CurrentPhase == EPlacePhase::StandUpEmpty)
    {
        AdvanceToPhase(EPlacePhase::Complete);
    }
}

FVector USK_Place::ResolveCurrentTargetLocation() const
{
    if (CurrentMode == EPlaceMode::LoadToUGV && TargetUGV.IsValid())
    {
        return TargetUGV->GetActorLocation();
    }
    if (CurrentMode == EPlaceMode::StackOnObject && TargetObject.IsValid())
    {
        return TargetObject->GetActorLocation();
    }
    return TargetLocation;
}

void USK_Place::CleanupPlaceRuntime(
    AMACharacter* Character,
    const bool bWasCancelled,
    bool& bOutSuccessToNotify,
    FString& InOutMessageToNotify)
{
    if (!Character)
    {
        NavigationService = nullptr;
        SourceObject.Reset();
        TargetObject.Reset();
        HeldObject.Reset();
        TargetUGV.Reset();
        return;
    }

    if (AMAHumanoidCharacter* Humanoid = GetHumanoidCharacter())
    {
        Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnBendDownComplete);
        Humanoid->OnBendAnimationComplete.RemoveDynamic(this, &USK_Place::OnStandUpComplete);
    }

    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToSourceCompleted);
        NavigationService->OnNavigationCompleted.RemoveDynamic(this, &USK_Place::OnNavigationToTargetCompleted);
        NavigationService->CancelNavigation();
    }

    if (UWorld* World = Character->GetWorld())
    {
        World->GetTimerManager().ClearTimer(PhaseTimerHandle);
    }
    PhaseTimerHandle.Invalidate();
    Character->ShowStatus(TEXT(""), 0.f);

    if (bWasCancelled && HeldObject.IsValid())
    {
        if (IMAPickupItem* Item = Cast<IMAPickupItem>(HeldObject.Get()))
        {
            Item->PlaceOnGround(Character->GetActorLocation());
        }
        HeldObject.Reset();
    }

    if (bWasCancelled && PlaceResultMessage.IsEmpty())
    {
        bOutSuccessToNotify = false;
        const FString PhaseStr = GetPhaseString();
        InOutMessageToNotify = FString::Printf(TEXT("Place cancelled: Stopped while %s"), *PhaseStr);

        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            SkillComp->GetFeedbackContextMutable().PlaceCancelledPhase = PhaseStr;
        }
    }

    NavigationService = nullptr;
    SourceObject.Reset();
    TargetObject.Reset();
    HeldObject.Reset();
    TargetUGV.Reset();
}

void USK_Place::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    ResetPlaceRuntimeState();

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        FailPlace(TEXT("Place failed: Character not found"), TEXT("Character not found"));
        return;
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        FailPlace(TEXT("Place failed: SkillComponent not found"), TEXT("SkillComponent not found"));
        return;
    }

    if (!InitializePlaceContext(*Character, *SkillComp))
    {
        return;
    }

    Character->ShowAbilityStatus(TEXT("Place"), TEXT("Starting..."));
    UpdatePhase();
}

void USK_Place::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();

    bool bSuccessToNotify = bPlaceSucceeded;
    FString MessageToNotify = PlaceResultMessage;

    CleanupPlaceRuntime(Character, bWasCancelled, bSuccessToNotify, MessageToNotify);

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::Place, bSuccessToNotify, MessageToNotify);
        }
    }
}
