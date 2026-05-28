#pragma once

#include "Agent/Skill/Infrastructure/MASkillTargetResolver.h"
#include "Core/Comm/Domain/MACommSkillTypes.h"
#include "Dom/JsonObject.h"

class AMACharacter;

namespace MAParamsHelper
{
bool ParseRawParams(const FString& RawParamsJson, TSharedPtr<FJsonObject>& OutJson);
bool ExtractDestPosition(const TSharedPtr<FJsonObject>& Json, FVector& OutPosition);
bool ExtractSearchArea(const TSharedPtr<FJsonObject>& Json, TArray<FVector>& OutBoundary);
bool ExtractTargetJson(const TSharedPtr<FJsonObject>& Json, const FString& FieldName, FString& OutTargetJson);
FString ExtractTaskId(const TSharedPtr<FJsonObject>& Json);
FString ExtractObjectId(const TSharedPtr<FJsonObject>& Json);
}

namespace MASkillParamsProcessorInternal
{
float ComputeHorizontalDistanceMeters(const AMACharacter* Agent, const FMAResolvedSkillTarget& Target);
}
