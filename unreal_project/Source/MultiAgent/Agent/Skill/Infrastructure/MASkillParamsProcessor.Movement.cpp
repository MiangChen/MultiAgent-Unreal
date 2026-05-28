#include "Agent/Skill/Infrastructure/MASkillParamsProcessor.h"
#include "Agent/Skill/Infrastructure/MASkillParamsProcessorInternal.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Agent/Skill/Infrastructure/MASkillSceneGraphBridge.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Core/Comm/Domain/MACommTypes.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"

void FMASkillParamsProcessor::ProcessNavigate(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Cmd) return;

    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();

    TSharedPtr<FJsonObject> ParamsJson;
    if (!MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessNavigate] %s: Failed to parse RawParamsJson"), *Agent->AgentLabel);
        return;
    }

    FVector TargetLocation;
    if (!MAParamsHelper::ExtractDestPosition(ParamsJson, TargetLocation))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessNavigate] %s: No dest position in params"), *Agent->AgentLabel);
        return;
    }

    const FVector OriginalTarget = TargetLocation;
    const bool bIsFlying = (Agent->AgentType == EMAAgentType::UAV || Agent->AgentType == EMAAgentType::FixedWingUAV);

    UWorld* World = Agent->GetWorld();
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Agent);

    const float ProbeRadius = 50.f;
    const float PushMargin = 200.f;

    if (bIsFlying)
    {
        TArray<FOverlapResult> Overlaps;
        if (World->OverlapMultiByChannel(
                Overlaps, TargetLocation, FQuat::Identity,
                ECC_WorldStatic, FCollisionShape::MakeSphere(ProbeRadius), QueryParams))
        {
            FVector AccumulatedPush = FVector::ZeroVector;

            for (const FOverlapResult& Overlap : Overlaps)
            {
                UPrimitiveComponent* Comp = Overlap.GetComponent();
                if (!Comp) continue;

                FMTDResult MTD;
                if (!Comp->ComputePenetration(MTD, FCollisionShape::MakeSphere(ProbeRadius), TargetLocation, FQuat::Identity))
                {
                    continue;
                }

                AccumulatedPush += MTD.Direction * (MTD.Distance + PushMargin);
            }

            if (!AccumulatedPush.IsNearlyZero())
            {
                TargetLocation += AccumulatedPush;

                UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Pushed out of collider, offset (%.0f, %.0f, %.0f)"),
                    *Agent->AgentLabel, AccumulatedPush.X, AccumulatedPush.Y, AccumulatedPush.Z);
            }
        }
    }

    if (bIsFlying)
    {
        float MinAltitude = 800.f;
        if (UMANavigationService* NavService = Agent->GetNavigationService())
        {
            MinAltitude = NavService->MinFlightAltitude;
        }
        if (TargetLocation.Z < MinAltitude)
        {
            TargetLocation.Z = MinAltitude;
        }
    }
    else
    {
        FCollisionQueryParams GroundQuery;
        GroundQuery.AddIgnoredActor(Agent);
        TArray<AActor*> AllPawns;
        UGameplayStatics::GetAllActorsOfClass(World, APawn::StaticClass(), AllPawns);
        GroundQuery.AddIgnoredActors(AllPawns);

        FHitResult HitResult;
        const FVector TraceStart(TargetLocation.X, TargetLocation.Y, 50000.f);
        const FVector TraceEnd(TargetLocation.X, TargetLocation.Y, -10000.f);

        if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, GroundQuery))
        {
            TargetLocation.Z = HitResult.Location.Z;
        }
    }

    if (!TargetLocation.Equals(OriginalTarget, 1.f))
    {
        UE_LOG(LogTemp, Log, TEXT("[ProcessNavigate] %s: Adjusted (%.0f,%.0f,%.0f) -> (%.0f,%.0f,%.0f)"),
            *Agent->AgentLabel,
            OriginalTarget.X, OriginalTarget.Y, OriginalTarget.Z,
            TargetLocation.X, TargetLocation.Y, TargetLocation.Z);
    }

    const FMASceneGraphNode NearestLandmark = FMASkillSceneGraphBridge::FindNearestLandmark(Agent, TargetLocation, 2000.f);
    if (NearestLandmark.IsValid())
    {
        Context.NearbyLandmarkLabel = NearestLandmark.Label;
        Context.NearbyLandmarkType = NearestLandmark.Type;
        Context.NearbyLandmarkDistance = FVector::Dist(TargetLocation, NearestLandmark.Center);
    }

    Params.TargetLocation = TargetLocation;
}
