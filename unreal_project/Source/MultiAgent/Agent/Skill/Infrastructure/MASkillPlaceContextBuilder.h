#pragma once

#include "CoreMinimal.h"
#include "Agent/Skill/Domain/MASkillTypes.h"

class AActor;
class AMACharacter;
class AMAUGVCharacter;
class UMASkillComponent;
struct FMAFeedbackContext;

struct MULTIAGENT_API FMAPlaceContextConfig
{
    EPlaceMode Mode = EPlaceMode::LoadToUGV;
    TWeakObjectPtr<AActor> SourceObject;
    TWeakObjectPtr<AActor> TargetObject;
    TWeakObjectPtr<AMAUGVCharacter> TargetUGV;
    FVector SourceLocation = FVector::ZeroVector;
    FVector TargetLocation = FVector::ZeroVector;
    FVector DropLocation = FVector::ZeroVector;
};

struct MULTIAGENT_API FMAPlaceContextBuildFeedback
{
    bool bSuccess = false;
    FMAPlaceContextConfig Config;
    FString FailureResultMessage;
    FString FailureReason;
    FString FailureStatusMessage;
};

struct MULTIAGENT_API FMAPlaceCompletionFeedback
{
    FString TargetName;
    FVector FinalLocation = FVector::ZeroVector;
};

struct MULTIAGENT_API FMASkillPlaceContextBuilder
{
    static FMAPlaceContextBuildFeedback Build(AMACharacter& Character, UMASkillComponent& SkillComponent);

    static FMAPlaceCompletionFeedback BuildCompletion(
        const FMAFeedbackContext& Context,
        const FMAPlaceContextConfig& Config);
};
