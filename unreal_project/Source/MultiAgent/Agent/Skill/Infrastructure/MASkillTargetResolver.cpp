#include "Agent/Skill/Infrastructure/MASkillTargetResolver.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Infrastructure/MASkillRuntimeBridge.h"
#include "Agent/Skill/Infrastructure/MAUESceneQuery.h"
#include "Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"
#include "Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FMASemanticLabel BuildSemanticLabel(const FMASemanticTarget& Target)
{
    FMASemanticLabel Label;
    Label.Class = Target.Class;
    Label.Type = Target.Type;
    Label.Features = Target.Features;
    return Label;
}
}

void FMASkillTargetResolver::ParseSemanticTargetFromJson(const FString& JsonStr, FMASemanticTarget& OutTarget)
{
    OutTarget = FMASemanticTarget();
    if (JsonStr.IsEmpty())
    {
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FMASkillTargetResolver] Failed to parse semantic target JSON: %s"), *JsonStr);
        return;
    }

    JsonObject->TryGetStringField(TEXT("class"), OutTarget.Class);
    JsonObject->TryGetStringField(TEXT("type"), OutTarget.Type);

    if (OutTarget.Class.Equals(TEXT("event"), ESearchCase::IgnoreCase))
    {
        FString EventType;
        if (JsonObject->TryGetStringField(TEXT("event_type"), EventType))
        {
            OutTarget.Class = TEXT("prop");
            OutTarget.Type = TEXT("vehicle");
            OutTarget.Features.Add(EventType, TEXT("true"));
            UE_LOG(LogTemp, Log, TEXT("[FMASkillTargetResolver] Converted event type '%s' to prop/vehicle target"), *EventType);
        }
        return;
    }

    const auto AppendFeatureObject = [&OutTarget](const TSharedPtr<FJsonObject>& FeaturesObject, const bool bOnlyIfMissing)
    {
        if (!FeaturesObject.IsValid())
        {
            return;
        }

        for (const auto& Pair : FeaturesObject->Values)
        {
            if (bOnlyIfMissing && OutTarget.Features.Contains(Pair.Key))
            {
                continue;
            }

            FString FeatureValue;
            if (Pair.Value->TryGetString(FeatureValue))
            {
                OutTarget.Features.Add(Pair.Key, FeatureValue);
            }
            else if (Pair.Value->Type == EJson::Boolean)
            {
                OutTarget.Features.Add(Pair.Key, Pair.Value->AsBool() ? TEXT("true") : TEXT("false"));
            }
            else if (Pair.Value->Type == EJson::Number)
            {
                OutTarget.Features.Add(Pair.Key, FString::Printf(TEXT("%g"), Pair.Value->AsNumber()));
            }
        }
    };

    const TSharedPtr<FJsonObject>* FeaturesObject = nullptr;
    if (JsonObject->TryGetObjectField(TEXT("features"), FeaturesObject))
    {
        AppendFeatureObject(*FeaturesObject, false);
    }

    const TSharedPtr<FJsonObject>* FeatureObject = nullptr;
    if (JsonObject->TryGetObjectField(TEXT("feature"), FeatureObject))
    {
        AppendFeatureObject(*FeatureObject, true);
    }
}

EPlaceMode FMASkillTargetResolver::DeterminePlaceMode(const FMASemanticTarget& SurfaceTarget)
{
    if (SurfaceTarget.Type.Contains(TEXT("UGV"), ESearchCase::IgnoreCase))
    {
        return EPlaceMode::LoadToUGV;
    }

    for (const auto& Pair : SurfaceTarget.Features)
    {
        if (Pair.Key.Contains(TEXT("UGV"), ESearchCase::IgnoreCase) ||
            Pair.Value.Contains(TEXT("UGV"), ESearchCase::IgnoreCase))
        {
            return EPlaceMode::LoadToUGV;
        }
    }

    if (SurfaceTarget.IsRobot())
    {
        return EPlaceMode::LoadToUGV;
    }

    if (SurfaceTarget.IsGround() || SurfaceTarget.Class.Contains(TEXT("ground"), ESearchCase::IgnoreCase))
    {
        return EPlaceMode::UnloadToGround;
    }

    return EPlaceMode::StackOnObject;
}

FMAResolvedSkillTarget FMASkillTargetResolver::ResolveTarget(
    AMACharacter& Agent,
    const FString& ObjectId,
    const FMASemanticTarget& Target,
    const TCHAR* LogContext)
{
    FMAResolvedSkillTarget Resolved;

    UWorld* World = FMASkillRuntimeBridge::ResolveWorld(&Agent);
    if (!World)
    {
        return Resolved;
    }

    const FMASemanticLabel TargetLabel = BuildSemanticLabel(Target);

    if (!TargetLabel.IsEmpty())
    {
        const FMAUESceneQueryResult Result = FMAUESceneQuery::FindObjectByLabel(World, TargetLabel);
        if (Result.bFound && Result.Actor)
        {
            Resolved.Actor = Result.Actor;
            Resolved.Name = Result.Name;
            Resolved.Location = Result.Location;
            Resolved.bFound = true;

            UE_LOG(LogTemp, Log, TEXT("[%s] %s: Found target '%s' by semantic label (UE5)"),
                LogContext, *Agent.AgentLabel, *Resolved.Name);
            return Resolved;
        }
    }

    if (UMASceneGraphManager* SceneGraphManager = FMASkillRuntimeBridge::ResolveSceneGraphManager(&Agent))
    {
        const TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode TargetNode;

        if (!ObjectId.IsEmpty())
        {
            TargetNode = FMASceneGraphQueryUseCases::FindNodeByIdOrLabel(AllNodes, ObjectId);
        }
        if (!TargetNode.IsValid() && !TargetLabel.IsEmpty())
        {
            TargetNode = FMASceneGraphQueryUseCases::FindNodeByLabel(AllNodes, TargetLabel);
        }

        if (TargetNode.IsValid())
        {
            Resolved.Id = TargetNode.Id;
            Resolved.Name = TargetNode.Label.IsEmpty() ? TargetNode.Id : TargetNode.Label;
            Resolved.Location = TargetNode.Center;
            Resolved.bFound = true;

            if (!TargetNode.Guid.IsEmpty())
            {
                Resolved.Actor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.Guid);
            }
            else if (TargetNode.GuidArray.Num() > 0)
            {
                Resolved.Actor = FMAUESceneQuery::FindActorByGuid(World, TargetNode.GuidArray[0]);
            }

            UE_LOG(LogTemp, Log, TEXT("[%s] %s: Found target '%s' by scene graph%s"),
                LogContext,
                *Agent.AgentLabel,
                *Resolved.Name,
                Resolved.Actor.IsValid() ? TEXT("") : TEXT(" (without Actor binding)"));
        }
    }

    return Resolved;
}
