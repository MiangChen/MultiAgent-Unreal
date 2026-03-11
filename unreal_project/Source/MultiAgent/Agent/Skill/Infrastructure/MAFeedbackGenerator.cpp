#include "MAFeedbackGenerator.h"

#include "Agent/Skill/Infrastructure/MASkillRuntimeBridge.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "Core/SceneGraph/Application/MASceneGraphQueryUseCases.h"

UMASceneGraphManager* FMAFeedbackGenerator::GetSceneGraphManager(AMACharacter* Agent)
{
    return FMASkillRuntimeBridge::ResolveSceneGraphManager(Agent);
}

void FMAFeedbackGenerator::AddCommonFieldsToFeedback(
    FMASkillExecutionFeedback& Feedback,
    UMASkillComponent* SkillComp,
    AMACharacter* Agent)
{
    if (!SkillComp || !Agent)
    {
        return;
    }

    const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
    if (!Context.TaskId.IsEmpty())
    {
        Feedback.Data.Add(TEXT("task_id"), Context.TaskId);
    }

    if (UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager(Agent))
    {
        const TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr->GetAllNodes();
        FString RobotId = FMASceneGraphQueryUseCases::GetIdByLabel(AllNodes, Agent->AgentID);
        if (RobotId.IsEmpty())
        {
            RobotId = Agent->AgentID;
        }

        Feedback.Data.Add(TEXT("robot_id"), RobotId);
        Feedback.Data.Add(TEXT("robot_label"), FMASceneGraphQueryUseCases::GetLabelById(AllNodes, RobotId));
        return;
    }

    Feedback.Data.Add(TEXT("robot_id"), Agent->AgentID);
    Feedback.Data.Add(TEXT("robot_label"), Agent->AgentLabel.IsEmpty() ? Agent->AgentID : Agent->AgentLabel);
}

FMASkillExecutionFeedback FMAFeedbackGenerator::Generate(
    AMACharacter* Agent,
    const EMACommand Command,
    const bool bSuccess,
    const FString& Message)
{
    FMASkillExecutionFeedback Feedback;
    if (!Agent)
    {
        Feedback.bSuccess = false;
        Feedback.Message = TEXT("Invalid agent");
        return Feedback;
    }

    Feedback.AgentId = Agent->AgentID;
    Feedback.SkillName = FMASkillRuntimeBridge::DescribeCommand(Command);
    Feedback.bSuccess = bSuccess;

    UMASkillComponent* SkillComp = Agent->GetSkillComponent();

    switch (Command)
    {
        case EMACommand::Navigate: GenerateNavigateFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Search: GenerateSearchFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Follow: GenerateFollowFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Charge: GenerateChargeFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Place: GeneratePlaceFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::TakeOff: GenerateTakeOffFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Land: GenerateLandFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::ReturnHome: GenerateReturnHomeFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::TakePhoto: GenerateTakePhotoFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Broadcast: GenerateBroadcastFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::HandleHazard: GenerateHandleHazardFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Guide: GenerateGuideFeedback(Feedback, Agent, SkillComp, bSuccess, Message); break;
        case EMACommand::Idle: GenerateIdleFeedback(Feedback, Agent, bSuccess, Message); break;
        default: Feedback.Message = Message; break;
    }

    return Feedback;
}

TSharedPtr<FJsonObject> FMAFeedbackGenerator::BuildNodeJsonFromSceneGraph(const FMASceneGraphNode& Node)
{
    TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject());
    NodeObject->SetStringField(TEXT("id"), Node.Id);

    TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject());
    PropertiesObject->SetStringField(TEXT("category"), Node.Category);
    PropertiesObject->SetStringField(TEXT("type"), Node.Type);
    PropertiesObject->SetStringField(TEXT("label"), Node.Label);

    if (!Node.LocationLabel.IsEmpty())
    {
        PropertiesObject->SetStringField(TEXT("location_label"), Node.LocationLabel);
    }

    if (Node.IsPickupItem())
    {
        PropertiesObject->SetBoolField(TEXT("is_carried"), Node.bIsCarried);
        if (!Node.CarrierId.IsEmpty())
        {
            PropertiesObject->SetStringField(TEXT("carrier_id"), Node.CarrierId);
        }
    }

    if (Node.bIsDynamic)
    {
        PropertiesObject->SetBoolField(TEXT("is_dynamic"), true);
    }

    for (const auto& Feature : Node.Features)
    {
        PropertiesObject->SetStringField(Feature.Key, Feature.Value);
    }

    NodeObject->SetObjectField(TEXT("properties"), PropertiesObject);

    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    ShapeObject->SetStringField(TEXT("type"), Node.ShapeType.IsEmpty() ? TEXT("point") : Node.ShapeType);

    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Node.Center.Z)));
    ShapeObject->SetArrayField(TEXT("center"), CenterArray);

    NodeObject->SetObjectField(TEXT("shape"), ShapeObject);
    return NodeObject;
}

TSharedPtr<FJsonObject> FMAFeedbackGenerator::BuildNodeJsonFromUE5Data(
    const FString& Id,
    const FString& Label,
    const FVector& Location,
    const FString& Type,
    const FString& Category,
    const TMap<FString, FString>& Features)
{
    TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject());
    NodeObject->SetStringField(TEXT("id"), Id);

    TSharedPtr<FJsonObject> PropertiesObject = MakeShareable(new FJsonObject());
    if (!Category.IsEmpty())
    {
        PropertiesObject->SetStringField(TEXT("category"), Category);
    }
    if (!Type.IsEmpty())
    {
        PropertiesObject->SetStringField(TEXT("type"), Type);
    }
    PropertiesObject->SetStringField(TEXT("label"), Label);

    for (const auto& Feature : Features)
    {
        PropertiesObject->SetStringField(Feature.Key, Feature.Value);
    }

    NodeObject->SetObjectField(TEXT("properties"), PropertiesObject);

    TSharedPtr<FJsonObject> ShapeObject = MakeShareable(new FJsonObject());
    ShapeObject->SetStringField(TEXT("type"), TEXT("point"));

    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
    ShapeObject->SetArrayField(TEXT("center"), CenterArray);

    NodeObject->SetObjectField(TEXT("shape"), ShapeObject);
    return NodeObject;
}
