#include "Agent/Skill/Infrastructure/MASkillRuntimeBridge.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Core/Command/Domain/MACommandNames.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UWorld* FMASkillRuntimeBridge::ResolveWorld(const AMACharacter* Agent)
{
    return Agent ? Agent->GetWorld() : nullptr;
}

UGameInstance* FMASkillRuntimeBridge::ResolveGameInstance(const AMACharacter* Agent)
{
    if (UWorld* World = ResolveWorld(Agent))
    {
        return World->GetGameInstance();
    }

    return nullptr;
}

FString FMASkillRuntimeBridge::DescribeCommand(const EMACommand Command)
{
    return FMACommandNames::ToString(Command);
}
