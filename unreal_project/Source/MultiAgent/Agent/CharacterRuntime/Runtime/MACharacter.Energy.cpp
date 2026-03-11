#include "MACharacter.h"

#include "Agent/CharacterRuntime/Application/MACharacterRuntimeUseCases.h"
#include "Agent/CharacterRuntime/Infrastructure/MACharacterRuntimeBridge.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"

bool AMACharacter::ShouldDrainEnergy() const
{
    return FMACharacterRuntimeBridge::ShouldDrainEnergy(this);
}

void AMACharacter::OnLowEnergyReturn()
{
    if (!SkillComponent) return;

    UE_LOG(LogTemp, Warning, TEXT("[%s] Low energy (%.0f%%), initiating return to base"),
        *AgentLabel, SkillComponent->GetEnergyPercent());

    const FMACharacterLowEnergyTriggerFeedback Feedback =
        FMACharacterRuntimeUseCases::BuildLowEnergyTrigger(
            FMACharacterRuntimeBridge::IsExecutionPaused(this));

    if (!Feedback.StatusMessage.IsEmpty())
    {
        ShowStatus(Feedback.StatusMessage, Feedback.StatusDuration);
    }

    if (Feedback.bShouldCancelSkills)
    {
        SkillComponent->CancelAllSkills();
    }

    if (Feedback.Action == EMACharacterLowEnergyAction::DeferUntilResume)
    {
        bPendingLowEnergyReturn = true;
    }

    if (Feedback.bShouldBindPauseStateChanged)
    {
        FMACharacterRuntimeBridge::BindPauseStateChanged(*this);
    }

    if (!Feedback.LogMessage.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("[%s] %s"), *AgentLabel, *Feedback.LogMessage);
    }

    if (Feedback.Action == EMACharacterLowEnergyAction::DeferUntilResume)
    {
        return;
    }

    ExecuteLowEnergyReturn();
}

void AMACharacter::OnPauseStateChanged(bool bPaused)
{
    const FMACharacterLowEnergyTriggerFeedback Feedback =
        FMACharacterRuntimeUseCases::BuildPauseResumeReaction(bPaused, bPendingLowEnergyReturn);
    if (Feedback.Action != EMACharacterLowEnergyAction::ExecuteReturn)
    {
        return;
    }

    if (Feedback.bShouldClearPendingReturn)
    {
        bPendingLowEnergyReturn = false;
    }

    if (Feedback.bShouldUnbindPauseStateChanged)
    {
        FMACharacterRuntimeBridge::UnbindPauseStateChanged(*this);
    }

    ExecuteLowEnergyReturn();
}

void AMACharacter::ExecuteLowEnergyReturn()
{
    if (!SkillComponent) return;

    const FMACharacterRuntimeFeedback ReturnPlan =
        FMACharacterRuntimeUseCases::BuildLowEnergyReturnPlan(
            NavigationService && NavigationService->bIsFlying);

    SkillComponent->CancelAllSkills();
    bIsLowEnergyReturning = true;
    SkillComponent->OnSkillCompleted.AddDynamic(this, &AMACharacter::OnReturnSkillCompleted);

    if (ReturnPlan.ReturnMode == EMACharacterReturnMode::ReturnHome)
    {
        SkillComponent->GetSkillParamsMutable().HomeLocation = InitialLocation;
        FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComponent, EMACommand::ReturnHome);
    }
    else
    {
        FMASkillActivationUseCases::PrepareAndActivateNavigate(*SkillComponent, InitialLocation);
    }

    UE_LOG(LogTemp, Log, TEXT("[%s] %s ([%.0f, %.0f, %.0f])"),
        *AgentLabel, *ReturnPlan.Message,
        InitialLocation.X, InitialLocation.Y, InitialLocation.Z);
}

void AMACharacter::OnReturnSkillCompleted(AMACharacter* Agent, bool bSuccess, const FString& Message)
{
    if (Agent != this) return;

    bIsLowEnergyReturning = false;

    if (SkillComponent)
    {
        SkillComponent->OnSkillCompleted.RemoveDynamic(this, &AMACharacter::OnReturnSkillCompleted);
    }

    UE_LOG(LogTemp, Log, TEXT("[%s] Low-energy return %s: %s"),
        *AgentLabel, bSuccess ? TEXT("completed") : TEXT("failed"), *Message);
}
