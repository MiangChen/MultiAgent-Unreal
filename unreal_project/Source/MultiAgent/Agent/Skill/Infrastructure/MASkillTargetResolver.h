#pragma once

#include "CoreMinimal.h"
#include "Agent/Skill/Domain/MASkillTypes.h"

class AActor;
class AMACharacter;

struct FMAResolvedSkillTarget
{
    TWeakObjectPtr<AActor> Actor;
    FString Name;
    FString Id;
    FVector Location = FVector::ZeroVector;
    bool bFound = false;

    bool HasActor() const { return Actor.IsValid(); }
};

struct MULTIAGENT_API FMASkillTargetResolver
{
    static void ParseSemanticTargetFromJson(const FString& JsonStr, FMASemanticTarget& OutTarget);
    static EPlaceMode DeterminePlaceMode(const FMASemanticTarget& SurfaceTarget);
    static FMAResolvedSkillTarget ResolveTarget(
        AMACharacter& Agent,
        const FString& ObjectId,
        const FMASemanticTarget& Target,
        const TCHAR* LogContext);
};
