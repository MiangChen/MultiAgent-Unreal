#include "Agent/Skill/Infrastructure/MASkillPlaceContextBuilder.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/CharacterRuntime/Runtime/MAUGVCharacter.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"

namespace
{
FMAPlaceContextBuildFeedback BuildLoadToUGVContext(UMASkillComponent& SkillComponent)
{
    FMAPlaceContextBuildFeedback Feedback;
    const FMASearchRuntimeResults& Results = SkillComponent.GetSearchResults();
    Feedback.Config.Mode = EPlaceMode::LoadToUGV;

    if (!Results.Object1Actor.IsValid())
    {
        Feedback.FailureResultMessage = TEXT("Place failed: Source object not found");
        Feedback.FailureReason = TEXT("Source object not found");
        Feedback.FailureStatusMessage = TEXT("[Place] Source object not found");
        return Feedback;
    }

    Feedback.Config.SourceObject = Results.Object1Actor;
    Feedback.Config.SourceLocation = Results.Object1Location;
    Feedback.Config.TargetUGV = Cast<AMAUGVCharacter>(Results.Object2Actor.Get());
    if (!Feedback.Config.TargetUGV.IsValid())
    {
        Feedback.FailureResultMessage = TEXT("Place failed: Target UGV not found");
        Feedback.FailureReason = TEXT("Target UGV not found");
        Feedback.FailureStatusMessage = TEXT("[Place] UGV not found");
        return Feedback;
    }

    Feedback.Config.TargetLocation = Feedback.Config.TargetUGV->GetActorLocation();
    Feedback.bSuccess = true;
    return Feedback;
}

FMAPlaceContextBuildFeedback BuildUnloadToGroundContext(AMACharacter& Character, UMASkillComponent& SkillComponent)
{
    FMAPlaceContextBuildFeedback Feedback;
    const FMASearchRuntimeResults& Results = SkillComponent.GetSearchResults();
    Feedback.Config.Mode = EPlaceMode::UnloadToGround;

    if (!Results.Object1Actor.IsValid())
    {
        Feedback.FailureResultMessage = TEXT("Place failed: Source object not found");
        Feedback.FailureReason = TEXT("Source object not found");
        Feedback.FailureStatusMessage = TEXT("[Place] Source object not found");
        return Feedback;
    }

    Feedback.Config.SourceObject = Results.Object1Actor;
    AActor* ParentActor = Results.Object1Actor->GetAttachParentActor();
    Feedback.Config.TargetUGV = Cast<AMAUGVCharacter>(ParentActor);
    Feedback.Config.SourceLocation = Feedback.Config.TargetUGV.IsValid()
        ? Feedback.Config.TargetUGV->GetActorLocation()
        : Results.Object1Actor->GetActorLocation();

    Feedback.Config.DropLocation = Results.Object2Location;
    if (Feedback.Config.DropLocation.IsZero())
    {
        Feedback.Config.DropLocation = Character.GetActorLocation() + Character.GetActorForwardVector() * 100.f;
    }

    Feedback.Config.TargetLocation = Feedback.Config.DropLocation;
    Feedback.bSuccess = true;
    return Feedback;
}

FMAPlaceContextBuildFeedback BuildStackOnObjectContext(UMASkillComponent& SkillComponent)
{
    FMAPlaceContextBuildFeedback Feedback;
    const FMASearchRuntimeResults& Results = SkillComponent.GetSearchResults();
    Feedback.Config.Mode = EPlaceMode::StackOnObject;

    if (!Results.Object1Actor.IsValid())
    {
        Feedback.FailureResultMessage = TEXT("Place failed: Source object not found");
        Feedback.FailureReason = TEXT("Source object (target) not found");
        Feedback.FailureStatusMessage = TEXT("[Place] Source object not found");
        return Feedback;
    }

    if (!Results.Object2Actor.IsValid())
    {
        Feedback.FailureResultMessage = TEXT("Place failed: Target object not found");
        Feedback.FailureReason = TEXT("Target object (surface_target) not found");
        Feedback.FailureStatusMessage = TEXT("[Place] Target object not found");
        return Feedback;
    }

    Feedback.Config.SourceObject = Results.Object1Actor;
    Feedback.Config.SourceLocation = Results.Object1Location;
    Feedback.Config.TargetObject = Results.Object2Actor;
    Feedback.Config.TargetLocation = Results.Object2Location;
    Feedback.bSuccess = true;
    return Feedback;
}
}

FMAPlaceContextBuildFeedback FMASkillPlaceContextBuilder::Build(
    AMACharacter& Character,
    UMASkillComponent& SkillComponent)
{
    const FMASkillParams& Params = SkillComponent.GetSkillParams();
    return Params.PlaceObject2.IsRobot()
        ? BuildLoadToUGVContext(SkillComponent)
        : (Params.PlaceObject2.IsGround()
            ? BuildUnloadToGroundContext(Character, SkillComponent)
            : BuildStackOnObjectContext(SkillComponent));
}

FMAPlaceCompletionFeedback FMASkillPlaceContextBuilder::BuildCompletion(
    const FMAFeedbackContext& Context,
    const FMAPlaceContextConfig& Config)
{
    FMAPlaceCompletionFeedback Feedback;

    switch (Config.Mode)
    {
        case EPlaceMode::LoadToUGV:
            Feedback.TargetName = Config.TargetUGV.IsValid() ? Config.TargetUGV->AgentLabel : TEXT("UGV");
            Feedback.FinalLocation = Config.TargetUGV.IsValid() ? Config.TargetUGV->GetActorLocation() : Config.TargetLocation;
            break;
        case EPlaceMode::UnloadToGround:
            Feedback.TargetName = TEXT("ground");
            Feedback.FinalLocation = Config.DropLocation;
            break;
        case EPlaceMode::StackOnObject:
            Feedback.TargetName = Context.PlaceTargetName.IsEmpty() ? TEXT("object") : Context.PlaceTargetName;
            Feedback.FinalLocation = Config.TargetObject.IsValid() ? Config.TargetObject->GetActorLocation() : Config.TargetLocation;
            break;
    }

    return Feedback;
}
