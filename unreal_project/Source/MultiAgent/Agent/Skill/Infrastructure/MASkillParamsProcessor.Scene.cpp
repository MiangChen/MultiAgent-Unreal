#include "Agent/Skill/Infrastructure/MASkillParamsProcessor.h"
#include "Agent/Skill/Infrastructure/MASkillParamsProcessorInternal.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Infrastructure/MASkillSceneGraphBridge.h"
#include "Agent/Skill/Infrastructure/MAUESceneQuery.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"
#include "Utils/MAGeometryUtils.h"

void FMASkillParamsProcessor::ProcessSearch(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();

    TSharedPtr<FJsonObject> ParamsJson;
    if (!MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessSearch] %s: Failed to parse RawParamsJson"), *Agent->AgentLabel);
        return;
    }

    if (MAParamsHelper::ExtractSearchArea(ParamsJson, Params.SearchBoundary))
    {
        Params.SearchScanWidth = FMAGeometryUtils::CalculateAdaptiveScanWidth(Params.SearchBoundary, 200.f, 1600.f);
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: SearchArea has %d vertices, ScanWidth=%.0f"),
            *Agent->AgentLabel, Params.SearchBoundary.Num(), Params.SearchScanWidth);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ProcessSearch] %s: No SearchArea provided"), *Agent->AgentLabel);
    }

    FString TargetJsonStr;
    if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
    {
        FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Params.SearchTarget);
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Parsed target - Class=%s, Type=%s, Features=%d"),
            *Agent->AgentLabel, *Params.SearchTarget.Class, *Params.SearchTarget.Type, Params.SearchTarget.Features.Num());
    }

    FString GoalType;
    if (ParamsJson->TryGetStringField(TEXT("goal_type"), GoalType))
    {
        Params.SearchMode = GoalType.Equals(TEXT("patrol"), ESearchCase::IgnoreCase)
            ? ESearchMode::Patrol
            : ESearchMode::Coverage;
    }
    else
    {
        Params.SearchMode = ESearchMode::Coverage;
    }

    double WaitTime = 2.0;
    if (ParamsJson->TryGetNumberField(TEXT("wait_time"), WaitTime))
    {
        Params.PatrolWaitTime = static_cast<float>(WaitTime);
    }

    int32 PatrolCycles = 1;
    if (ParamsJson->TryGetNumberField(TEXT("patrol_cycles"), WaitTime))
    {
        PatrolCycles = static_cast<int32>(WaitTime);
        if (PatrolCycles < 1) PatrolCycles = 1;
    }
    Params.PatrolCycleLimit = PatrolCycles;

    UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: SearchMode=%s, PatrolWaitTime=%.1f, PatrolCycleLimit=%d"),
        *Agent->AgentLabel,
        Params.SearchMode == ESearchMode::Patrol ? TEXT("Patrol") : TEXT("Coverage"),
        Params.PatrolWaitTime,
        Params.PatrolCycleLimit);

    FMASemanticLabel SearchLabel;
    SearchLabel.Class = Params.SearchTarget.Class;
    SearchLabel.Type = Params.SearchTarget.Type;
    SearchLabel.Features = Params.SearchTarget.Features;

    UWorld* World = Agent->GetWorld();
    if (!World) return;

    const TArray<FMASceneGraphNode> AllNodes = FMASkillSceneGraphBridge::LoadAllNodes(Agent);
    bool bFoundInSceneGraph = false;

    if (AllNodes.Num() > 0 && Params.SearchBoundary.Num() >= 3)
    {
        const TArray<FMASceneGraphNode> FoundNodes = FMASceneGraphQueryUseCases::FindNodesInBoundary(
            AllNodes, SearchLabel, Params.SearchBoundary);

        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Scene graph query found %d nodes"),
            *Agent->AgentLabel, FoundNodes.Num());

        for (const FMASceneGraphNode& Node : FoundNodes)
        {
            const FString ObjectName = Node.Label.IsEmpty() ? Node.Id : Node.Label;
            Context.FoundObjects.Add(ObjectName);
            Context.FoundLocations.Add(Node.Center);

            const int32 Index = Context.FoundObjects.Num() - 1;
            if (!Node.Type.IsEmpty())
            {
                Context.ObjectAttributes.Add(FString::Printf(TEXT("object_%d_type"), Index), Node.Type);
            }
            if (!Node.Category.IsEmpty())
            {
                Context.ObjectAttributes.Add(FString::Printf(TEXT("object_%d_category"), Index), Node.Category);
            }
            for (const auto& Feature : Node.Features)
            {
                Context.ObjectAttributes.Add(FString::Printf(TEXT("object_%d_%s"), Index, *Feature.Key), Feature.Value);
            }
        }

        bFoundInSceneGraph = FoundNodes.Num() > 0;
    }

    if (!bFoundInSceneGraph && Params.SearchBoundary.Num() >= 3)
    {
        UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Falling back to UE5 scene query"), *Agent->AgentLabel);

        FMASemanticLabel UE5Label;
        UE5Label.Class = SearchLabel.Class;
        UE5Label.Type = SearchLabel.Type;
        UE5Label.Features = SearchLabel.Features;

        const TArray<FMAUESceneQueryResult> Results = FMAUESceneQuery::FindObjectsInBoundary(
            World, UE5Label, Params.SearchBoundary);

        for (const FMAUESceneQueryResult& Result : Results)
        {
            Context.FoundObjects.Add(Result.Name);
            Context.FoundLocations.Add(Result.Location);

            const int32 Index = Context.FoundObjects.Num() - 1;
            if (!Result.Label.Type.IsEmpty())
            {
                Context.ObjectAttributes.Add(FString::Printf(TEXT("object_%d_type"), Index), Result.Label.Type);
            }
            for (const auto& Feature : Result.Label.Features)
            {
                Context.ObjectAttributes.Add(FString::Printf(TEXT("object_%d_%s"), Index, *Feature.Key), Feature.Value);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[ProcessSearch] %s: Total found %d objects"),
        *Agent->AgentLabel, Context.FoundObjects.Num());
}

void FMASkillParamsProcessor::ProcessPlace(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent || !SkillComp || !Cmd) return;

    FMASkillParams& Params = SkillComp->GetSkillParamsMutable();
    FMAFeedbackContext& Context = SkillComp->GetFeedbackContextMutable();
    FMASearchRuntimeResults& SearchResults = SkillComp->GetSearchResultsMutable();
    FMASkillRuntimeTargets& RuntimeTargets = SkillComp->GetSkillRuntimeTargetsMutable();
    SearchResults.Reset();

    TSharedPtr<FJsonObject> ParamsJson;
    if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
    {
        FString TargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("target"), TargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(TargetJsonStr, Params.PlaceObject1);
        }

        FString SurfaceTargetJsonStr;
        if (MAParamsHelper::ExtractTargetJson(ParamsJson, TEXT("surface_target"), SurfaceTargetJsonStr))
        {
            FMASkillTargetResolver::ParseSemanticTargetFromJson(SurfaceTargetJsonStr, Params.PlaceObject2);
        }
    }

    const EPlaceMode PlaceMode = FMASkillTargetResolver::DeterminePlaceMode(Params.PlaceObject2);
    UE_LOG(LogTemp, Log, TEXT("[ProcessPlace] %s: PlaceMode=%d"), *Agent->AgentLabel, static_cast<int32>(PlaceMode));

    UWorld* World = Agent->GetWorld();
    if (!World) return;

    const TArray<FMASceneGraphNode> AllNodes = FMASkillSceneGraphBridge::LoadAllNodes(Agent);

    FMASemanticLabel Label1;
    Label1.Class = Params.PlaceObject1.Class;
    Label1.Type = Params.PlaceObject1.Type;
    Label1.Features = Params.PlaceObject1.Features;

    if (!Label1.IsEmpty())
    {
        bool bFoundInSceneGraph = false;
        if (AllNodes.Num() > 0)
        {
            const FMASceneGraphNode Object1Node = FMASceneGraphQueryUseCases::FindNodeByLabel(AllNodes, Label1);
            if (Object1Node.IsValid() && Object1Node.IsPickupItem())
            {
                Context.PlacedObjectName = Object1Node.Label.IsEmpty() ? Object1Node.Id : Object1Node.Label;
                SearchResults.Object1Location = Object1Node.Center;
                Context.ObjectAttributes.Add(TEXT("object1_node_id"), Object1Node.Id);

                const FMAUESceneQueryResult ActorResult = FMAUESceneQuery::FindObjectByLabel(World, Label1);
                if (ActorResult.bFound && ActorResult.Actor)
                {
                    SearchResults.Object1Actor = ActorResult.Actor;
                }

                bFoundInSceneGraph = true;
            }
        }

        if (!bFoundInSceneGraph)
        {
            const FMAUESceneQueryResult Result1 = FMAUESceneQuery::FindObjectByLabel(World, Label1);
            if (Result1.bFound)
            {
                Context.PlacedObjectName = Result1.Name;
                SearchResults.Object1Actor = Result1.Actor;
                SearchResults.Object1Location = Result1.Location;
            }
        }
    }

    FMASemanticLabel Label2;
    Label2.Class = Params.PlaceObject2.Class;
    Label2.Type = Params.PlaceObject2.Type;
    Label2.Features = Params.PlaceObject2.Features;

    switch (PlaceMode)
    {
        case EPlaceMode::UnloadToGround:
            Context.PlaceTargetName = TEXT("ground");
            SearchResults.Object2Location = Agent->GetActorLocation();
            break;

        case EPlaceMode::LoadToUGV:
        {
            const FString TargetRobotLabel = Label2.Features.Contains(TEXT("label")) ? Label2.Features[TEXT("label")] : TEXT("");
            bool bFoundInSceneGraph = false;

            if (AllNodes.Num() > 0 && !TargetRobotLabel.IsEmpty())
            {
                const FMASemanticLabel RobotSemanticLabel = FMASemanticLabel::MakeRobot(TargetRobotLabel);
                const FMASceneGraphNode RobotNode = FMASceneGraphQueryUseCases::FindNodeByLabel(AllNodes, RobotSemanticLabel);

                if (RobotNode.IsValid() && RobotNode.IsRobot())
                {
                    Context.PlaceTargetName = RobotNode.Label.IsEmpty() ? RobotNode.Id : RobotNode.Label;
                    SearchResults.Object2Location = RobotNode.Center;
                    Context.ObjectAttributes.Add(TEXT("object2_node_id"), RobotNode.Id);

                    if (AMACharacter* Robot = FMAUESceneQuery::FindRobotByLabel(World, TargetRobotLabel))
                    {
                        SearchResults.Object2Actor = Robot;
                    }

                    bFoundInSceneGraph = true;
                }
            }

            if (!bFoundInSceneGraph)
            {
                if (AMACharacter* Robot = FMAUESceneQuery::FindRobotByLabel(World, TargetRobotLabel))
                {
                    Context.PlaceTargetName = Robot->AgentLabel;
                    SearchResults.Object2Actor = Robot;
                    SearchResults.Object2Location = Robot->GetActorLocation();
                }
            }
            break;
        }

        case EPlaceMode::StackOnObject:
        {
            bool bFoundInSceneGraph = false;
            if (AllNodes.Num() > 0)
            {
                const FMASceneGraphNode Object2Node = FMASceneGraphQueryUseCases::FindNodeByLabel(AllNodes, Label2);
                if (Object2Node.IsValid() && Object2Node.IsPickupItem())
                {
                    Context.PlaceTargetName = Object2Node.Label.IsEmpty() ? Object2Node.Id : Object2Node.Label;
                    SearchResults.Object2Location = Object2Node.Center;
                    Context.ObjectAttributes.Add(TEXT("object2_node_id"), Object2Node.Id);

                    const FMAUESceneQueryResult ActorResult = FMAUESceneQuery::FindObjectByLabel(World, Label2);
                    if (ActorResult.bFound && ActorResult.Actor)
                    {
                        SearchResults.Object2Actor = ActorResult.Actor;
                    }

                    bFoundInSceneGraph = true;
                }
            }

            if (!bFoundInSceneGraph)
            {
                const FMAUESceneQueryResult Result2 = FMAUESceneQuery::FindObjectByLabel(World, Label2);
                if (Result2.bFound)
                {
                    Context.PlaceTargetName = Result2.Name;
                    SearchResults.Object2Actor = Result2.Actor;
                    SearchResults.Object2Location = Result2.Location;
                }
            }
            break;
        }
    }
}
