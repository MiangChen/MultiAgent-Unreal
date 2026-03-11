#pragma once

#include "CoreMinimal.h"
#include "Core/Command/Domain/MACommandTypes.h"

class AMACharacter;
class UGameInstance;
class UMASceneGraphManager;
class UWorld;

struct MULTIAGENT_API FMASkillRuntimeBridge
{
    static UWorld* ResolveWorld(const AMACharacter* Agent);
    static UGameInstance* ResolveGameInstance(const AMACharacter* Agent);
    static UMASceneGraphManager* ResolveSceneGraphManager(const AMACharacter* Agent);
    static FString DescribeCommand(EMACommand Command);
};
