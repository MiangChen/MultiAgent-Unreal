#include "Agent/Skill/Infrastructure/MASkillRuntimeBridge.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Core/Command/Runtime/MACommandManager.h"
#include "Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"
#include "Core/SceneGraph/Runtime/MASceneGraphManager.h"
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

UMASceneGraphManager* FMASkillRuntimeBridge::ResolveSceneGraphManager(const AMACharacter* Agent)
{
    return Agent ? FMASceneGraphBootstrap::Resolve(Agent) : nullptr;
}

FString FMASkillRuntimeBridge::DescribeCommand(const EMACommand Command)
{
    return UMACommandManager::CommandToString(Command);
}
