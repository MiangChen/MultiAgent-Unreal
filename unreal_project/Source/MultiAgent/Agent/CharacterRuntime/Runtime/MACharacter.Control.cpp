#include "MACharacter.h"

#include "Agent/CharacterRuntime/Application/MACharacterRuntimeUseCases.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

bool AMACharacter::TryNavigateTo(FVector Destination)
{
    return SkillComponent
        ? FMASkillActivationUseCases::PrepareAndActivateNavigate(*SkillComponent, Destination)
        : false;
}

void AMACharacter::CancelNavigation()
{
    if (SkillComponent)
    {
        FMASkillActivationUseCases::CancelCommand(*SkillComponent, EMACommand::Navigate);
    }
}

bool AMACharacter::TryFollowActor(AActor* TargetActor, float FollowDistance)
{
    return SkillComponent
        ? FMASkillActivationUseCases::PrepareAndActivateFollow(*SkillComponent, TargetActor, FollowDistance)
        : false;
}

void AMACharacter::StopFollowing()
{
    if (SkillComponent)
    {
        FMASkillActivationUseCases::CancelCommand(*SkillComponent, EMACommand::Follow);
    }
}

void AMACharacter::SetDirectControl(bool bEnabled)
{
    const FMACharacterDirectControlFeedback Feedback =
        FMACharacterRuntimeUseCases::BuildDirectControlTransition(bIsUnderDirectControl, bEnabled);
    if (Feedback.Transition == EMACharacterDirectControlTransition::None)
    {
        return;
    }

    bIsUnderDirectControl = Feedback.Transition == EMACharacterDirectControlTransition::Enable;
    if (Feedback.bShouldCancelAIMovement)
    {
        CancelAIMovement();
    }
    GetCharacterMovement()->bOrientRotationToMovement = Feedback.bOrientRotationToMovement;
}

void AMACharacter::CancelAIMovement()
{
    if (AAIController* AIController = Cast<AAIController>(GetController()))
    {
        AIController->StopMovement();
        if (UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent())
        {
            PathFollowing->AbortMove(*this, FPathFollowingResultFlags::UserAbort);
        }
    }

    if (SkillComponent)
    {
        FMASkillActivationUseCases::CancelCommand(*SkillComponent, EMACommand::Navigate);
        FMASkillActivationUseCases::CancelCommand(*SkillComponent, EMACommand::Follow);
    }

    bIsMoving = false;
}

void AMACharacter::ApplyDirectMovement(FVector WorldDirection)
{
    if (!bIsUnderDirectControl) return;

    if (!WorldDirection.IsNearlyZero())
    {
        if (AAIController* AIController = Cast<AAIController>(GetController()))
        {
            if (AIController->GetMoveStatus() != EPathFollowingStatus::Idle)
            {
                AIController->StopMovement();
            }
        }
    }

    AddMovementInput(WorldDirection, 1.0f, false);
}
