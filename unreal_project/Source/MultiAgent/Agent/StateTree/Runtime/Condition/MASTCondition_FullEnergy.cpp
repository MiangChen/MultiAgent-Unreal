// MASTCondition_FullEnergy.cpp
// 从 SkillComponent 获取能量信息

#include "MASTCondition_FullEnergy.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "StateTreeExecutionContext.h"

bool FMASTCondition_FullEnergy::TestCondition(FStateTreeExecutionContext& Context) const
{
    AActor* Owner = Cast<AActor>(Context.GetOwner());
    if (!Owner) return false;

    UMASkillComponent* SkillComp = Owner->FindComponentByClass<UMASkillComponent>();
    if (!SkillComp) return false;

    return SkillComp->GetEnergyPercent() >= FullThreshold;
}
