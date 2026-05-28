#include "Agent/Skill/Infrastructure/MASkillParamsProcessor.h"
#include "Agent/Skill/Infrastructure/MASkillParamsProcessorInternal.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"

void FMASkillParamsProcessor::ProcessTakePhoto(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();

    FString ObjectId;
    FMASemanticTarget Target;

    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);

        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: object_id='%s', target(Class=%s, Type=%s)"),
        *Agent->AgentLabel, *ObjectId, *Target.Class, *Target.Type);

    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessTakePhoto"));
    const float RobotTargetDistanceM = MASkillParamsProcessorInternal::ComputeHorizontalDistanceMeters(Agent, ResolvedTarget);
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;

    Params.CommonTargetObjectId = ResolvedTarget.Id;
    Params.CommonTarget = Target;
    RuntimeTargets.PhotoTargetActor = ResolvedTarget.Actor;

    static const TSet<FString> WideViewTypes = { TEXT("intersection"), TEXT("building"), TEXT("area") };
    if (WideViewTypes.Contains(Target.Type.ToLower()))
    {
        Params.PhotoFOVOverride = 110.f;
    }

    Context.bPhotoTargetFound = ResolvedTarget.bFound;
    Context.PhotoTargetName = ResolvedTarget.Name;
    Context.PhotoTargetId = ResolvedTarget.Id;
    Context.TargetLocation = TargetLocation;
    Context.PhotoRobotTargetDistance = RobotTargetDistanceM;

    UE_LOG(LogTemp, Log, TEXT("[ProcessTakePhoto] %s: Target found=%s, name='%s', id='%s', distance=%.2fm, Actor=%s"),
        *Agent->AgentLabel,
        Context.bPhotoTargetFound ? TEXT("true") : TEXT("false"),
        *ResolvedTarget.Name, *ResolvedTarget.Id, RobotTargetDistanceM,
        ResolvedTarget.Actor.IsValid() ? TEXT("valid") : TEXT("null"));
}

void FMASkillParamsProcessor::ProcessBroadcast(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();

    FString ObjectId;
    FMASemanticTarget Target;
    FString Message;

    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);

        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }

        ParamsJson->TryGetStringField(TEXT("message"), Message);
    }

    UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: object_id='%s', target(Class=%s, Type=%s), message='%s'"),
        *Agent->AgentLabel, *ObjectId, *Target.Class, *Target.Type, *Message);

    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessBroadcast"));
    const float RobotTargetDistanceM = MASkillParamsProcessorInternal::ComputeHorizontalDistanceMeters(Agent, ResolvedTarget);
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;

    Params.CommonTargetObjectId = ResolvedTarget.Id;
    Params.CommonTarget = Target;
    Params.BroadcastMessage = Message;
    RuntimeTargets.BroadcastTargetActor = ResolvedTarget.Actor;

    Context.bBroadcastTargetFound = ResolvedTarget.bFound;
    Context.BroadcastTargetName = ResolvedTarget.Name;
    Context.BroadcastMessage = Message;
    Context.TargetLocation = TargetLocation;
    Context.BroadcastRobotTargetDistance = RobotTargetDistanceM;

    UE_LOG(LogTemp, Log, TEXT("[ProcessBroadcast] %s: Target found=%s, name='%s', message='%s', distance=%.2fm, Actor=%s"),
        *Agent->AgentLabel,
        Context.bBroadcastTargetFound ? TEXT("true") : TEXT("false"),
        *ResolvedTarget.Name, *Message, RobotTargetDistanceM,
        ResolvedTarget.Actor.IsValid() ? TEXT("valid") : TEXT("null"));
}

void FMASkillParamsProcessor::ProcessHandleHazard(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();

    FString ObjectId;
    FMASemanticTarget Target;

    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
    }

    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessHandleHazard"));
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;

    Params.CommonTargetObjectId = ResolvedTarget.Id;
    Params.CommonTarget = Target;
    RuntimeTargets.HazardTargetActor = ResolvedTarget.Actor;

    Context.bHazardTargetFound = ResolvedTarget.bFound;
    Context.HazardTargetName = ResolvedTarget.Name;
    Context.HazardTargetId = ResolvedTarget.Id;
    Context.TargetLocation = TargetLocation;

    UE_LOG(LogTemp, Log, TEXT("[ProcessHandleHazard] %s: Target found=%s, name='%s', Actor=%s"),
        *Agent->AgentLabel, Context.bHazardTargetFound ? TEXT("true") : TEXT("false"),
        *ResolvedTarget.Name, ResolvedTarget.Actor.IsValid() ? TEXT("valid") : TEXT("null"));
}

void FMASkillParamsProcessor::ProcessGuide(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();

    FString ObjectId;
    FMASemanticTarget Target;
    FVector Destination = FVector::ZeroVector;

    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
        MAParamsHelper::ExtractDestPosition(ParamsJson, Destination);
    }

    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessGuide"));
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;

    Context.bGuideTargetFound = ResolvedTarget.Actor.IsValid();
    Context.GuideTargetName = ResolvedTarget.Name;
    Context.GuideTargetId = ResolvedTarget.Id;
    Context.GuideDestination = Destination;
    Context.TargetLocation = TargetLocation;

    Params.GuideTargetObjectId = ResolvedTarget.Id;
    Params.GuideTarget = Target;
    Params.GuideDestination = Destination;
    RuntimeTargets.GuideTargetActor = ResolvedTarget.Actor;

    UE_LOG(LogTemp, Log, TEXT("[ProcessGuide] %s: Target found=%s, name='%s', destination=(%.0f, %.0f, %.0f)"),
        *Agent->AgentLabel, ResolvedTarget.Actor.IsValid() ? TEXT("true") : TEXT("false"), *ResolvedTarget.Name,
        Destination.X, Destination.Y, Destination.Z);
}

void FMASkillParamsProcessor::ProcessFollow(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp || !Cmd) return;

    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;

    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();

    FString ObjectId;
    FMASemanticTarget Target;

    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        ObjectId = MAParamsHelper::ExtractObjectId(ParamsJson);
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Target);
        }
    }

    const FMAResolvedSkillTarget ResolvedTarget = FMASkillTargetResolver::ResolveTarget(
        *Agent, ObjectId, Target, TEXT("ProcessFollow"));
    AActor* TargetActor = ResolvedTarget.Actor.Get();
    const FVector TargetLocation = ResolvedTarget.Actor.IsValid()
        ? ResolvedTarget.Actor->GetActorLocation()
        : ResolvedTarget.Location;

    Context.bFollowTargetFound = (TargetActor != nullptr);
    Context.FollowTargetName = ResolvedTarget.Name;
    Context.FollowTargetId = ObjectId;
    Context.TargetName = ResolvedTarget.Name;
    Context.TargetLocation = TargetLocation;

    if (TargetActor)
    {
        RuntimeTargets.FollowTarget = TargetActor;
        Context.FollowTargetDistance = FVector::Dist(Agent->GetActorLocation(), TargetActor->GetActorLocation());
        UE_LOG(LogTemp, Log, TEXT("[ProcessFollow] %s: Target '%s' at distance %.0f"),
            *Agent->AgentLabel, *ResolvedTarget.Name, Context.FollowTargetDistance);
    }
    else
    {
        Context.FollowErrorReason = TEXT("Target Actor not found");
        UE_LOG(LogTemp, Warning, TEXT("[ProcessFollow] %s: Target not found (ObjectId=%s, Class=%s)"),
            *Agent->AgentLabel, *ObjectId, *Target.Class);
    }
}
