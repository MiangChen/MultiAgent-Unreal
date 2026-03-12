#include "Agent/Skill/Infrastructure/MASkillParamsProcessor.h"
#include "Agent/Skill/Infrastructure/MASkillParamsProcessorInternal.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Infrastructure/MASkillSceneGraphBridge.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"
#include "Environment/Entity/MAChargingStation.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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

    const float SafetyMargin = 500.f;
    const float SearchRadius = 3000.f;
    float MinSafeHeight = RequestedHeight;
    float MaxBuildingHeight = 0.f;
    FString TallestBuildingLabel;

    const TArray<FMASceneGraphNode> AllNodes = FMASkillSceneGraphBridge::LoadAllNodes(Agent);
    if (AllNodes.Num() > 0)
    {
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

    const TArray<FMASceneGraphNode> AllNodes = FMASkillSceneGraphBridge::LoadAllNodes(Agent);
    if (AllNodes.Num() > 0)
    {
        if (FMASkillSceneGraphBridge::IsPointInsideBuilding(Agent, LandLocation))
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
                const FMASceneGraphNode NearestLandmark = FMASkillSceneGraphBridge::FindNearestLandmark(Agent, LandLocation, 2000.f);
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
    if (!Agent) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();

    FVector HomeLocation = Agent->InitialLocation;
    FString HomeLandmark;

    // ReturnHome 的 home anchor 采用角色开始游戏时记录的初始位置。
    // 当前语义下不再对 return_home 做运行期目标覆写或动态测高。
    (void)Cmd;

    const FMASceneGraphNode HomeNearestLandmark = FMASkillSceneGraphBridge::FindNearestLandmark(Agent, HomeLocation, 2000.f);
    if (HomeNearestLandmark.IsValid())
    {
        HomeLandmark = HomeNearestLandmark.Label;
    }

    Params.HomeLocation = HomeLocation;
    Params.LandHeight = HomeLocation.Z;
    Context.HomeLocationFromSceneGraph = HomeLocation;
    Context.HomeLandmarkLabel = HomeLandmark;
    Context.bHomeLocationFromSceneGraph = false;
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
    bool bFoundInSceneGraph = false;

    const TArray<FMASceneGraphNode> AllNodes = FMASkillSceneGraphBridge::LoadAllNodes(Agent);
    if (AllNodes.Num() > 0)
    {
        const FMASemanticLabel ChargingStationLabel = FMASemanticLabel::MakeChargingStation();
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
