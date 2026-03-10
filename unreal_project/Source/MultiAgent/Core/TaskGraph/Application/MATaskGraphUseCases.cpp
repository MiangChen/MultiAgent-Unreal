#include "Core/TaskGraph/Application/MATaskGraphUseCases.h"

#include "Core/TaskGraph/Infrastructure/MATaskGraphJsonCodec.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
FTaskGraphLoadResult MakeFailureResult(const FString& Message)
{
    FTaskGraphLoadResult Result;
    Result.bSuccess = false;
    Result.Feedback.Kind = ETaskGraphFeedbackKind::Error;
    Result.Feedback.Message = Message;
    Result.Feedback.bSuccess = false;
    return Result;
}

FTaskGraphLoadResult MakeSuccessResult(const FMATaskGraphData& Data, const FString& Message)
{
    FTaskGraphLoadResult Result;
    Result.bSuccess = true;
    Result.Data = Data;
    Result.Feedback.Kind = ETaskGraphFeedbackKind::Success;
    Result.Feedback.Message = Message;
    Result.Feedback.bSuccess = true;
    return Result;
}

FString BuildSummaryMessage(const FMATaskGraphData& Data, const FString& Prefix)
{
    return FString::Printf(TEXT("%s: %d nodes, %d edges"), *Prefix, Data.Nodes.Num(), Data.Edges.Num());
}
}

FTaskGraphLoadResult FTaskGraphUseCases::ParseJson(const FString& JsonString)
{
    FMATaskGraphData Data;
    FString ErrorMessage;
    if (!FTaskGraphJsonCodec::TryParseJson(JsonString, Data, ErrorMessage))
    {
        return MakeFailureResult(ErrorMessage);
    }

    return MakeSuccessResult(Data, BuildSummaryMessage(Data, TEXT("Task graph loaded")));
}

FTaskGraphLoadResult FTaskGraphUseCases::ParseResponseJson(const FString& JsonString)
{
    FMATaskGraphData Data;
    FString ErrorMessage;
    if (!FTaskGraphJsonCodec::TryParseResponseJson(JsonString, Data, ErrorMessage))
    {
        return MakeFailureResult(ErrorMessage);
    }

    return MakeSuccessResult(Data, BuildSummaryMessage(Data, TEXT("Task graph response parsed")));
}

FTaskGraphLoadResult FTaskGraphUseCases::ParseFlexibleJson(const FString& JsonString)
{
    FMATaskGraphData Data;
    FString ErrorMessage;
    if (!FTaskGraphJsonCodec::TryParseJson(JsonString, Data, ErrorMessage))
    {
        return MakeFailureResult(ErrorMessage);
    }

    return MakeSuccessResult(Data, BuildSummaryMessage(Data, TEXT("Task graph parsed")));
}

FTaskGraphLoadResult FTaskGraphUseCases::LoadResponseExampleFile(const FString& FilePath)
{
    if (!FPaths::FileExists(FilePath))
    {
        return MakeFailureResult(FString::Printf(TEXT("Mock data file not found: %s"), *FilePath));
    }

    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        return MakeFailureResult(FString::Printf(TEXT("Unable to read file: %s"), *FilePath));
    }

    return ParseResponseJson(JsonContent);
}

FString FTaskGraphUseCases::Serialize(const FMATaskGraphData& Data)
{
    return FTaskGraphJsonCodec::Serialize(Data);
}
