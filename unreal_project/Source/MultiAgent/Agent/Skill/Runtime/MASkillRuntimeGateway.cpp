#include "Agent/Skill/Runtime/MASkillRuntimeGateway.h"

#include "Agent/Skill/Runtime/MASkillComponent.h"

bool FMASkillRuntimeGateway::PrepareAndActivateNavigate(UMASkillComponent& SkillComponent, const FVector TargetLocation)
{
    SkillComponent.PrepareNavigateActivation(TargetLocation);
    return ActivatePreparedCommand(SkillComponent, EMACommand::Navigate);
}

bool FMASkillRuntimeGateway::PrepareAndActivateFollow(
    UMASkillComponent& SkillComponent,
    AActor* TargetActor,
    const float FollowDistance)
{
    if (!TargetActor)
    {
        return false;
    }

    SkillComponent.PrepareFollowActivation(TargetActor, FollowDistance);
    return ActivatePreparedCommand(SkillComponent, EMACommand::Follow);
}

bool FMASkillRuntimeGateway::PrepareAndActivateCharge(UMASkillComponent& SkillComponent)
{
    SkillComponent.PrepareChargeActivation();
    return ActivatePreparedCommand(SkillComponent, EMACommand::Charge);
}

bool FMASkillRuntimeGateway::ActivatePreparedCommand(UMASkillComponent& SkillComponent, const EMACommand Command)
{
    switch (Command)
    {
        case EMACommand::Navigate:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.NavigateSkillHandle, TEXT("Navigate"));
        case EMACommand::Follow:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.FollowSkillHandle, TEXT("Follow"));
        case EMACommand::Charge:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.ChargeSkillHandle, TEXT("Charge"));
        case EMACommand::Search:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.SearchSkillHandle, TEXT("Search"));
        case EMACommand::Place:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.PlaceSkillHandle, TEXT("Place"));
        case EMACommand::TakeOff:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.TakeOffSkillHandle, TEXT("TakeOff"));
        case EMACommand::Land:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.LandSkillHandle, TEXT("Land"));
        case EMACommand::ReturnHome:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.ReturnHomeSkillHandle, TEXT("ReturnHome"));
        case EMACommand::TakePhoto:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.TakePhotoSkillHandle, TEXT("TakePhoto"));
        case EMACommand::Broadcast:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.BroadcastSkillHandle, TEXT("Broadcast"));
        case EMACommand::HandleHazard:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.HandleHazardSkillHandle, TEXT("HandleHazard"));
        case EMACommand::Guide:
            return SkillComponent.TryActivateSkillHandle(SkillComponent.GuideSkillHandle, TEXT("Guide"));
        case EMACommand::Idle:
            SkillComponent.NotifySkillCompleted(true, TEXT("Idle state entered"));
            return true;
        case EMACommand::None:
        default:
            SkillComponent.NotifySkillCompleted(false, TEXT("Unknown command type"));
            return false;
    }
}

void FMASkillRuntimeGateway::CancelCommand(UMASkillComponent& SkillComponent, const EMACommand Command)
{
    switch (Command)
    {
        case EMACommand::Navigate:
            SkillComponent.CancelSkillHandleIfValid(SkillComponent.NavigateSkillHandle);
            break;
        case EMACommand::Follow:
            SkillComponent.CancelSkillHandleIfValid(SkillComponent.FollowSkillHandle);
            break;
        case EMACommand::Search:
            SkillComponent.CancelSkillHandleIfValid(SkillComponent.SearchSkillHandle);
            break;
        case EMACommand::Place:
            SkillComponent.CancelSkillHandleIfValid(SkillComponent.PlaceSkillHandle);
            break;
        case EMACommand::ReturnHome:
            SkillComponent.CancelSkillHandleIfValid(SkillComponent.ReturnHomeSkillHandle);
            break;
        default:
            break;
    }
}
