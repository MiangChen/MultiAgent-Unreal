#include "Core/SkillAllocation/Application/MASkillAllocationUseCases.h"

#include "Core/SkillAllocation/Infrastructure/MASkillAllocationJsonCodec.h"

FMASkillAllocationParseFeedback FMASkillAllocationUseCases::ParseJson(const FString& JsonString)
{
    FMASkillAllocationParseFeedback Feedback;
    Feedback.bSuccess = FMASkillAllocationJsonCodec::Deserialize(JsonString, Feedback.Data, Feedback.Message);
    Feedback.bWarning = Feedback.bSuccess && Feedback.Message.StartsWith(TEXT("[Warning]"));
    return Feedback;
}

bool FMASkillAllocationUseCases::ParseJson(
    const FString& JsonString,
    FMASkillAllocationData& OutData,
    FString& OutError)
{
    const FMASkillAllocationParseFeedback Feedback = ParseJson(JsonString);
    if (Feedback.bSuccess)
    {
        OutData = Feedback.Data;
    }
    OutError = Feedback.Message;
    return Feedback.bSuccess;
}

FString FMASkillAllocationUseCases::SerializeJson(const FMASkillAllocationData& Data)
{
    return FMASkillAllocationJsonCodec::Serialize(Data);
}

FMASkillAllocationMessage FMASkillAllocationUseCases::BuildMessage(const FMASkillAllocationData& Data)
{
    FMASkillAllocationMessage Message;
    Message.Name = Data.Name;
    Message.Description = Data.Description;
    Message.DataJson = SerializeJson(Data);
    return Message;
}

FMASkillAllocationSkillListBuildFeedback FMASkillAllocationUseCases::BuildSkillListMessage(
    const FMASkillAllocationData& Data)
{
    FMASkillAllocationSkillListBuildFeedback Feedback;
    Feedback.bSuccess = TryBuildSkillListMessage(Data, Feedback.SkillList, Feedback.Message);
    return Feedback;
}

bool FMASkillAllocationUseCases::TryBuildSkillListMessage(
    const FMASkillAllocationData& Data,
    FMASkillListMessage& OutMessage,
    FString& OutError)
{
    OutMessage = FMASkillListMessage();
    OutError.Empty();

    if (Data.Data.Num() == 0)
    {
        OutError = TEXT("技能列表为空");
        return false;
    }

    TArray<int32> TimeSteps;
    Data.Data.GetKeys(TimeSteps);
    TimeSteps.Sort();
    OutMessage.TotalTimeSteps = TimeSteps.Num();

    for (int32 TimeStep : TimeSteps)
    {
        FMATimeStepCommands TimeStepCommands;
        TimeStepCommands.TimeStep = TimeStep;

        const FMATimeStepData* TimeStepData = Data.Data.Find(TimeStep);
        if (!TimeStepData)
        {
            continue;
        }

        for (const auto& Pair : TimeStepData->RobotSkills)
        {
            FMAAgentSkillCommand Command;
            Command.AgentId = Pair.Key;
            Command.SkillName = Pair.Value.SkillName;
            Command.Params.RawParamsJson = Pair.Value.ParamsJson;
            TimeStepCommands.Commands.Add(Command);
        }

        OutMessage.TimeSteps.Add(TimeStepCommands);
    }

    return true;
}
