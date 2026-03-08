#pragma once

#include "CoreMinimal.h"
#include "MACommBaseTypes.h"
#include "MACommResponseTypes.generated.h"

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAReviewResponseMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReviewResponse")
    bool bApproved = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReviewResponse")
    FString ModifiedDataJson;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReviewResponse")
    FString RejectionReason;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReviewResponse")
    int64 Timestamp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReviewResponse", Meta = (IgnoreForMemberInitializationTest))
    FString MessageId;

    FMAReviewResponseMessage()
        : Timestamp(MACommDomainDefaults::CurrentTimestampMs())
        , MessageId(MACommDomainDefaults::NewMessageId())
    {
    }

    FMAReviewResponseMessage(bool bInApproved, const FString& InModifiedDataJson = TEXT(""), const FString& InRejectionReason = TEXT(""))
        : bApproved(bInApproved)
        , ModifiedDataJson(InModifiedDataJson)
        , RejectionReason(InRejectionReason)
        , Timestamp(MACommDomainDefaults::CurrentTimestampMs())
        , MessageId(MACommDomainDefaults::NewMessageId())
    {
    }
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMADecisionResponseMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DecisionResponse")
    FString Decision;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DecisionResponse")
    FString DecisionDataJson;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DecisionResponse")
    FString UserFeedback;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DecisionResponse")
    int64 Timestamp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DecisionResponse", Meta = (IgnoreForMemberInitializationTest))
    FString MessageId;

    FMADecisionResponseMessage()
        : Timestamp(MACommDomainDefaults::CurrentTimestampMs())
        , MessageId(MACommDomainDefaults::NewMessageId())
    {
    }

    FMADecisionResponseMessage(const FString& InDecision, const FString& InDecisionDataJson = TEXT(""), const FString& InUserFeedback = TEXT(""))
        : Decision(InDecision)
        , DecisionDataJson(InDecisionDataJson)
        , UserFeedback(InUserFeedback)
        , Timestamp(MACommDomainDefaults::CurrentTimestampMs())
        , MessageId(MACommDomainDefaults::NewMessageId())
    {
    }
};
