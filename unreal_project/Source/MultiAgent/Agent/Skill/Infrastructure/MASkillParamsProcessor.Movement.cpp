#include "Agent/Skill/Infrastructure/MASkillParamsProcessor.h"
#include "Agent/Skill/Infrastructure/MASkillParamsProcessorInternal.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Agent/Skill/Infrastructure/MASkillRuntimeBridge.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Core/Comm/Domain/MACommTypes.h"
#include "Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"
#include "Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "Environment/Entity/MAChargingStation.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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

                FVector Push = MTD.Direction * (MTD.Distance + PushMargin);

                if (!bIsFlying)
                {
                    Push.Z = 0.f;
                    if (Push.IsNearlyZero())
                    {
                        FVector Away = TargetLocation - Comp->GetComponentLocation();
                        Away.Z = 0.f;
                        if (Away.IsNearlyZero())
                        {
                            Away = FVector(1.f, 0.f, 0.f);
                        }
                        Push = Away.GetSafeNormal() * (MTD.Distance + PushMargin);
                    }
                }

                AccumulatedPush += Push;
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

    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);

    if (SceneGraphManager)
    {
        const FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(TargetLocation, 2000.f);
        if (NearestLandmark.IsValid())
        {
            Context.NearbyLandmarkLabel = NearestLandmark.Label;
            Context.NearbyLandmarkType = NearestLandmark.Type;
            Context.NearbyLandmarkDistance = FVector::Dist(TargetLocation, NearestLandmark.Center);
        }
    }

    Params.TargetLocation = TargetLocation;
}

void FMASkillParamsProcessor::ProcessTakeOff(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp) return;

    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();

    float RequestedHeight = Params.TakeOffHeight;

    if (Cmd)
    {
        TSharedPtr<FJsonObject> ParamsJson;
        if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
        {
            FVector DestPosition;
            if (MAParamsHelper::ExtractDestPosition(ParamsJson, DestPosition) && DestPosition.Z > 0.f)
            {
                RequestedHeight = DestPosition.Z;
            }

            double Height = 0.0;
            if (ParamsJson->TryGetNumberField(TEXT("height"), Height) && Height > 0.0)
            {
                RequestedHeight = static_cast<float>(Height);
            }
        }
    }

    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);

    const float SafetyMargin = 500.f;
    const float SearchRadius = 3000.f;
    float MinSafeHeight = RequestedHeight;
    float MaxBuildingHeight = 0.f;
    FString TallestBuildingLabel;

    if (SceneGraphManager)
    {
        const TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        const TArray<FMASceneGraphNode> Buildings = FMASceneGraphQueryUseCases::GetAllBuildings(AllNodes);
        const FVector AgentLocation = Agent->GetActorLocation();

        for (const FMASceneGraphNode& Building : Buildings)
        {
            const float Distance = FVector::Dist2D(AgentLocation, Building.Center);
            if (Distance > SearchRadius)
            {
                continue;
            }

            float BuildingHeight = 0.f;
            if (!Building.RawJson.IsEmpty())
            {
                TSharedPtr<FJsonObject> JsonObject;
                const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Building.RawJson);
                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    const TSharedPtr<FJsonObject>* PropertiesObject;
                    if (JsonObject->TryGetObjectField(TEXT("properties"), PropertiesObject))
                    {
                        double Height = 0.0;
                        if ((*PropertiesObject)->TryGetNumberField(TEXT("height"), Height))
                        {
                            BuildingHeight = static_cast<float>(Height);
                        }
                    }

                    if (BuildingHeight <= 0.f)
                    {
                        const TSharedPtr<FJsonObject>* ShapeObject;
                        if (JsonObject->TryGetObjectField(TEXT("shape"), ShapeObject))
                        {
                            double ZMax = 0.0;
                            if ((*ShapeObject)->TryGetNumberField(TEXT("z_max"), ZMax))
                            {
                                BuildingHeight = static_cast<float>(ZMax);
                            }
                        }
                    }
                }
            }

            if (BuildingHeight <= 0.f)
            {
                BuildingHeight = 1000.f;
            }

            if (BuildingHeight > MaxBuildingHeight)
            {
                MaxBuildingHeight = BuildingHeight;
                TallestBuildingLabel = Building.Label;
            }
        }

        if (MaxBuildingHeight > 0.f)
        {
            MinSafeHeight = MaxBuildingHeight + SafetyMargin;
        }
    }

    float FinalHeight = RequestedHeight;
    const bool bHeightAdjusted = RequestedHeight < MinSafeHeight;
    if (bHeightAdjusted)
    {
        FinalHeight = MinSafeHeight;
    }

    Params.TakeOffHeight = FinalHeight;
    Context.TakeOffTargetHeight = FinalHeight;
    Context.TakeOffMinSafeHeight = MinSafeHeight;
    Context.TakeOffNearbyBuildingLabel = TallestBuildingLabel;
    Context.TakeOffNearbyBuildingHeight = MaxBuildingHeight;
    Context.bTakeOffHeightAdjusted = bHeightAdjusted;
}

void FMASkillParamsProcessor::ProcessLand(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp) return;

    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();

    UWorld* World = Agent->GetWorld();
    const FVector AgentLocation = Agent->GetActorLocation();
    FVector LandLocation = FVector(AgentLocation.X, AgentLocation.Y, 0.f);
    float LandHeight = 0.f;
    FString GroundType = TEXT("unknown");
    FString NearbyLandmark;
    bool bLocationSafe = true;

    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
    if (SceneGraphManager)
    {
        const TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();

        if (SceneGraphManager->IsPointInsideBuilding(LandLocation))
        {
            bLocationSafe = false;
            GroundType = TEXT("building_interior");

            const TArray<FMASceneGraphNode> Roads = FMASceneGraphQueryUseCases::GetAllRoads(AllNodes);
            const TArray<FMASceneGraphNode> Intersections = FMASceneGraphQueryUseCases::GetAllIntersections(AllNodes);

            float MinDistance = TNumericLimits<float>::Max();
            FMASceneGraphNode NearestSafeNode;
            for (const FMASceneGraphNode& Road : Roads)
            {
                const float Distance = FVector::Dist2D(AgentLocation, Road.Center);
                if (Distance < MinDistance)
                {
                    MinDistance = Distance;
                    NearestSafeNode = Road;
                }
            }
            for (const FMASceneGraphNode& Intersection : Intersections)
            {
                const float Distance = FVector::Dist2D(AgentLocation, Intersection.Center);
                if (Distance < MinDistance)
                {
                    MinDistance = Distance;
                    NearestSafeNode = Intersection;
                }
            }

            if (NearestSafeNode.IsValid())
            {
                LandLocation = NearestSafeNode.Center;
                bLocationSafe = true;
                GroundType = NearestSafeNode.IsRoad() ? TEXT("road") : (NearestSafeNode.IsIntersection() ? TEXT("intersection") : GroundType);
                NearbyLandmark = NearestSafeNode.Label;
            }
        }
        else
        {
            const TArray<FMASceneGraphNode> Roads = FMASceneGraphQueryUseCases::GetAllRoads(AllNodes);
            for (const FMASceneGraphNode& Road : Roads)
            {
                if (FVector::Dist2D(AgentLocation, Road.Center) < 500.f)
                {
                    GroundType = TEXT("road");
                    NearbyLandmark = Road.Label;
                    break;
                }
            }

            if (GroundType == TEXT("unknown"))
            {
                const TArray<FMASceneGraphNode> Intersections = FMASceneGraphQueryUseCases::GetAllIntersections(AllNodes);
                for (const FMASceneGraphNode& Intersection : Intersections)
                {
                    if (FVector::Dist2D(AgentLocation, Intersection.Center) < 500.f)
                    {
                        GroundType = TEXT("intersection");
                        NearbyLandmark = Intersection.Label;
                        break;
                    }
                }
            }

            if (GroundType == TEXT("unknown"))
            {
                const FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(LandLocation, 2000.f);
                if (NearestLandmark.IsValid())
                {
                    NearbyLandmark = NearestLandmark.Label;
                    GroundType = TEXT("open_area");
                }
            }
        }
    }

    bool bHasDestPosition = false;
    FVector DestPosition;
    if (Cmd)
    {
        TSharedPtr<FJsonObject> ParamsJson;
        if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
        {
            if (MAParamsHelper::ExtractDestPosition(ParamsJson, DestPosition))
            {
                bHasDestPosition = true;
            }
        }
    }

    if (bHasDestPosition)
    {
        LandHeight = DestPosition.Z;
        LandLocation = FVector(DestPosition.X, DestPosition.Y, LandHeight);
    }
    else
    {
        const FVector Start(LandLocation.X, LandLocation.Y, AgentLocation.Z);
        const FVector End(LandLocation.X, LandLocation.Y, -10000.f);

        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(Agent);

        if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
        {
            LandHeight = HitResult.Location.Z;
        }
    }

    Params.LandHeight = LandHeight;
    Context.LandTargetLocation = FVector(LandLocation.X, LandLocation.Y, LandHeight);
    Context.LandGroundType = GroundType;
    Context.LandNearbyLandmarkLabel = NearbyLandmark;
    Context.bLandLocationSafe = bLocationSafe;
}

void FMASkillParamsProcessor::ProcessReturnHome(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp) return;

    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();

    UWorld* World = Agent->GetWorld();
    FVector HomeLocation = Agent->InitialLocation;
    FString HomeLandmark;

    if (Cmd)
    {
        TSharedPtr<FJsonObject> ParamsJson;
        if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
        {
            FVector DestPosition;
            if (MAParamsHelper::ExtractDestPosition(ParamsJson, DestPosition))
            {
                HomeLocation = DestPosition;
            }
        }
    }

    if (UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent))
    {
        const FMASceneGraphNode NearestLandmark = SceneGraphManager->FindNearestLandmark(HomeLocation, 2000.f);
        if (NearestLandmark.IsValid())
        {
            HomeLandmark = NearestLandmark.Label;
        }
    }

    float LandHeight = 50.f;
    const FVector Start(HomeLocation.X, HomeLocation.Y, 50000.f);
    const FVector End(HomeLocation.X, HomeLocation.Y, -10000.f);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Agent);

    if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
    {
        LandHeight = HitResult.Location.Z + 50.f;
    }

    Params.HomeLocation = HomeLocation;
    Params.LandHeight = LandHeight;
    Context.HomeLocationFromSceneGraph = HomeLocation;
    Context.HomeLandmarkLabel = HomeLandmark;
}

void FMASkillParamsProcessor::ProcessCharge(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!SkillComp) return;

    AMACharacter* Agent = Cast<AMACharacter>(SkillComp->GetOwner());
    if (!Agent || !Agent->GetWorld()) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();

    Context.EnergyBefore = SkillComp->GetEnergyPercent();

    UWorld* World = Agent->GetWorld();
    UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
    bool bFoundInSceneGraph = false;

    if (SceneGraphManager)
    {
        const FMASemanticLabel ChargingStationLabel = FMASemanticLabel::MakeChargingStation();
        const TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        const FMASceneGraphNode NearestStation = FMASceneGraphQueryUseCases::FindNearestNode(
            AllNodes, ChargingStationLabel, Agent->GetActorLocation());

        if (NearestStation.IsValid() && NearestStation.IsChargingStation())
        {
            Params.ChargingStationLocation = NearestStation.Center;
            Params.ChargingStationId = NearestStation.Id;
            Params.bChargingStationFound = true;

            Context.ChargingStationId = NearestStation.Label.IsEmpty() ? NearestStation.Id : NearestStation.Label;
            Context.ChargingStationLocation = NearestStation.Center;
            Context.bChargingStationFound = true;
            bFoundInSceneGraph = true;
        }
    }

    if (!bFoundInSceneGraph)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(World, AMAChargingStation::StaticClass(), FoundActors);

        if (FoundActors.Num() > 0)
        {
            AActor* NearestActor = nullptr;
            float MinDistanceSq = TNumericLimits<float>::Max();

            for (AActor* Actor : FoundActors)
            {
                const float DistanceSq = FVector::DistSquared(Agent->GetActorLocation(), Actor->GetActorLocation());
                if (DistanceSq < MinDistanceSq)
                {
                    MinDistanceSq = DistanceSq;
                    NearestActor = Actor;
                }
            }

            if (NearestActor)
            {
                Params.ChargingStationLocation = NearestActor->GetActorLocation();
                Params.ChargingStationId = NearestActor->GetName();
                Params.bChargingStationFound = true;

                Context.ChargingStationId = NearestActor->GetName();
                Context.ChargingStationLocation = NearestActor->GetActorLocation();
                Context.bChargingStationFound = true;
            }
        }
    }

    if (!Params.bChargingStationFound)
    {
        Context.bChargingStationFound = false;
        Context.ChargeErrorReason = TEXT("no charging station available");
        UE_LOG(LogTemp, Warning, TEXT("[ProcessCharge] %s: %s"), *Agent->AgentLabel, *Context.ChargeErrorReason);
    }
}
