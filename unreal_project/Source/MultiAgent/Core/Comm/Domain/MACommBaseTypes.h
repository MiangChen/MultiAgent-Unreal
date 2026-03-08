#pragma once

#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "MACommBaseTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMACommTypes, Log, All);

namespace MACommDomainDefaults
{
    inline int64 CurrentTimestampMs()
    {
        return FDateTime::UtcNow().ToUnixTimestamp() * 1000 + FDateTime::UtcNow().GetMillisecond();
    }

    inline FString NewMessageId()
    {
        return FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
    }
}

UENUM(BlueprintType)
enum class EMACommMessageType : uint8
{
    UIInput         UMETA(DisplayName = "UI Input"),
    ButtonEvent     UMETA(DisplayName = "Button Event"),
    TaskFeedback    UMETA(DisplayName = "Task Feedback"),
    WorldState      UMETA(DisplayName = "World State"),
    SceneChange     UMETA(DisplayName = "Scene Change"),

    TaskGraph       UMETA(DisplayName = "Task Graph"),
    WorldModelGraph UMETA(DisplayName = "World Model Graph"),
    SkillList       UMETA(DisplayName = "Skill List"),
    QueryRequest    UMETA(DisplayName = "Query Request"),
    SkillAllocation UMETA(DisplayName = "Skill Allocation"),
    SkillStatusUpdate UMETA(DisplayName = "Skill Status Update"),

    Custom          UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class EMAMessageCategory : uint8
{
    Instruction UMETA(DisplayName = "Instruction"),
    Review      UMETA(DisplayName = "Review"),
    Decision    UMETA(DisplayName = "Decision"),
    Platform    UMETA(DisplayName = "Platform")
};

UENUM(BlueprintType)
enum class EMAMessageDirection : uint8
{
    PythonToUE5 UMETA(DisplayName = "Python to UE5"),
    UE5ToPython UMETA(DisplayName = "UE5 to Python")
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAMessageEnvelope
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    EMACommMessageType MessageType = EMACommMessageType::Custom;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    EMAMessageCategory MessageCategory = EMAMessageCategory::Platform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    EMAMessageDirection Direction = EMAMessageDirection::UE5ToPython;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    int64 Timestamp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    FString TimestampISO;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    FString MessageId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope")
    FString PayloadJson;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAUIInputMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIInput")
    FString InputSourceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIInput")
    FString InputContent;

    FMAUIInputMessage() {}

    FMAUIInputMessage(const FString& InSourceId, const FString& InContent)
        : InputSourceId(InSourceId), InputContent(InContent) {}
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAButtonEventMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ButtonEvent")
    FString WidgetName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ButtonEvent")
    FString ButtonId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ButtonEvent")
    FString ButtonText;

    FMAButtonEventMessage() {}

    FMAButtonEventMessage(const FString& InWidgetName, const FString& InButtonId, const FString& InButtonText)
        : WidgetName(InWidgetName), ButtonId(InButtonId), ButtonText(InButtonText) {}
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskFeedbackMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    FString TaskId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    FString Status;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    float DurationSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    float EnergyConsumed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskFeedback")
    FString ErrorMessage;
};
