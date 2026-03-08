#pragma once

#include "CoreMinimal.h"
#include "MACommSkillTypes.generated.h"

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillParams_Comm
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString RawParamsJson;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAAgentSkillCommand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString AgentId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FString SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FMASkillParams_Comm Params;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATimeStepCommands
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 TimeStep = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TArray<FMAAgentSkillCommand> Commands;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillListMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillList")
    TArray<FMATimeStepCommands> TimeSteps;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillList")
    int32 TotalTimeSteps = 0;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillFeedback_Comm
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString AgentId;

    UPROPERTY(BlueprintReadWrite)
    FString SkillName;

    UPROPERTY(BlueprintReadWrite)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite)
    FString Message;

    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> Data;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATimeStepFeedbackMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 TimeStep = 0;

    UPROPERTY(BlueprintReadWrite)
    TArray<FMASkillFeedback_Comm> Feedbacks;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillListCompletedMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    bool bCompleted = false;

    UPROPERTY(BlueprintReadWrite)
    bool bInterrupted = false;

    UPROPERTY(BlueprintReadWrite)
    int32 CompletedTimeSteps = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 TotalTimeSteps = 0;

    UPROPERTY(BlueprintReadWrite)
    FString Message;

    UPROPERTY(BlueprintReadWrite)
    TArray<FMATimeStepFeedbackMessage> AllTimeStepFeedbacks;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMASkillListReceived, const FMASkillListMessage&, SkillList, bool, bExecutable);
