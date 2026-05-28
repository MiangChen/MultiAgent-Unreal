#include "Agent/Skill/Infrastructure/MAFeedbackGeneratorInternal.h"

#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Serialization/JsonSerializer.h"

void FMAFeedbackGenerator::GenerateSearchFeedback(
    FMASkillExecutionFeedback& Feedback,
    AMACharacter* Agent,
    UMASkillComponent* SkillComp,
    const bool bSuccess,
    const FString& Message)
{
    AddCommonFieldsToFeedback(Feedback, SkillComp, Agent);

    if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const int32 FoundCount = Context.FoundObjects.Num();

        Feedback.Data.Add(TEXT("found_count"), FString::FromInt(FoundCount));
        Feedback.Data.Add(TEXT("found"), FoundCount > 0 ? TEXT("true") : TEXT("false"));

        if (!Context.SearchAreaToken.IsEmpty())
        {
            Feedback.Data.Add(TEXT("area_token"), Context.SearchAreaToken);
        }

        MAFeedbackGeneratorInternal::AddDurationSecondsField(Feedback, Context.SearchDurationSeconds);

        if (FoundCount > 0)
        {
            const TArray<FMASceneGraphNode> AllNodes = LoadSceneGraphNodes(Agent);

            TArray<TSharedPtr<FJsonValue>> DiscoveredNodesArray;
            TArray<TSharedPtr<FJsonValue>> FoundIdsArray;

            for (int32 Index = 0; Index < FoundCount; ++Index)
            {
                const FString& ObjectName = Context.FoundObjects[Index];
                const FVector ObjectLocation = Context.FoundLocations.IsValidIndex(Index)
                    ? Context.FoundLocations[Index]
                    : FVector::ZeroVector;

                TSharedPtr<FJsonObject> NodeJson;
                FString NodeId;

                if (AllNodes.Num() > 0)
                {
                    const FMASceneGraphNode Node = FMASceneGraphQueryUseCases::FindNodeByIdOrLabel(AllNodes, ObjectName);
                    if (Node.IsValid())
                    {
                        NodeJson = BuildNodeJsonFromSceneGraph(Node);
                        NodeId = Node.Id;
                    }
                }

                if (!NodeJson.IsValid())
                {
                    NodeId = FString::Printf(TEXT("temp_%s_%d"), *ObjectName, Index);
                    NodeJson = BuildNodeJsonFromUE5Data(NodeId, ObjectName, ObjectLocation, TEXT(""), TEXT(""), TMap<FString, FString>());
                }

                DiscoveredNodesArray.Add(MakeShareable(new FJsonValueObject(NodeJson)));
                FoundIdsArray.Add(MakeShareable(new FJsonValueString(NodeId)));
            }

            FString DiscoveredNodesJsonString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&DiscoveredNodesJsonString);
            FJsonSerializer::Serialize(DiscoveredNodesArray, Writer);
            Feedback.Data.Add(TEXT("discovered_nodes"), DiscoveredNodesJsonString);

            FString FoundIdsJsonString;
            TSharedRef<TJsonWriter<>> IdsWriter = TJsonWriterFactory<>::Create(&FoundIdsJsonString);
            FJsonSerializer::Serialize(FoundIdsArray, IdsWriter);
            Feedback.Data.Add(TEXT("found_ids"), FoundIdsJsonString);

            Feedback.Data.Add(TEXT("found_objects"), FString::Join(Context.FoundObjects, TEXT(", ")));
        }
    }

    if (!Message.IsEmpty())
    {
        Feedback.Message = Message;
    }
    else if (SkillComp)
    {
        const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
        const int32 FoundCount = Context.FoundObjects.Num();

        if (FoundCount > 0)
        {
            Feedback.Message = FoundCount == 1
                ? FString::Printf(TEXT("Search completed, found %s"), *Context.FoundObjects[0])
                : FString::Printf(TEXT("Search completed, found %d objects"), FoundCount);
        }
        else
        {
            Feedback.Message = TEXT("Search completed, no target found");
        }
    }
    else
    {
        Feedback.Message = bSuccess ? TEXT("Search completed") : TEXT("Search failed");
    }
}
