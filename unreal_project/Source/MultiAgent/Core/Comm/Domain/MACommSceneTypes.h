#pragma once

#include "CoreMinimal.h"
#include "../../Types/MATaskGraphTypes.h"
#include "MACommBaseTypes.h"
#include "MACommSceneTypes.generated.h"

UENUM(BlueprintType)
enum class EMASceneChangeType : uint8
{
    AddNode         UMETA(DisplayName = "Add Node"),
    DeleteNode      UMETA(DisplayName = "Delete Node"),
    EditNode        UMETA(DisplayName = "Edit Node"),
    AddGoal         UMETA(DisplayName = "Add Goal"),
    DeleteGoal      UMETA(DisplayName = "Delete Goal"),
    AddZone         UMETA(DisplayName = "Add Zone"),
    DeleteZone      UMETA(DisplayName = "Delete Zone"),
    AddEdge         UMETA(DisplayName = "Add Edge"),
    EditEdge        UMETA(DisplayName = "Edit Edge"),
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASceneChangeMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SceneChange")
    EMASceneChangeType ChangeType = EMASceneChangeType::AddNode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SceneChange")
    FString Payload;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SceneChange")
    int64 Timestamp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SceneChange", Meta = (IgnoreForMemberInitializationTest))
    FString MessageId;

    FMASceneChangeMessage()
        : Timestamp(MACommDomainDefaults::CurrentTimestampMs())
        , MessageId(MACommDomainDefaults::NewMessageId())
    {
    }

    FMASceneChangeMessage(EMASceneChangeType InType, const FString& InPayload)
        : ChangeType(InType)
        , Payload(InPayload)
        , Timestamp(MACommDomainDefaults::CurrentTimestampMs())
        , MessageId(MACommDomainDefaults::NewMessageId())
    {
    }
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillAllocationMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString DataJson;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    int64 Timestamp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation", Meta = (IgnoreForMemberInitializationTest))
    FString MessageId;

    FMASkillAllocationMessage()
        : Timestamp(MACommDomainDefaults::CurrentTimestampMs())
        , MessageId(MACommDomainDefaults::NewMessageId())
    {
    }

    FMASkillAllocationMessage(const FString& InName, const FString& InDescription, const FString& InDataJson)
        : Name(InName)
        , Description(InDescription)
        , DataJson(InDataJson)
        , Timestamp(MACommDomainDefaults::CurrentTimestampMs())
        , MessageId(MACommDomainDefaults::NewMessageId())
    {
    }
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillStatusUpdateMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillStatus")
    int32 TimeStep = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillStatus")
    FString RobotId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillStatus")
    FString Status;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillStatus")
    FString ErrorMessage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillStatus")
    int64 Timestamp = 0;

    FMASkillStatusUpdateMessage()
        : Timestamp(MACommDomainDefaults::CurrentTimestampMs())
    {
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMASkillAllocationDataReceived, const FMASkillAllocationData&, AllocationData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMASkillStatusUpdateReceived, const FMASkillStatusUpdateMessage&, StatusUpdate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMARequestUserCommandReceived);
