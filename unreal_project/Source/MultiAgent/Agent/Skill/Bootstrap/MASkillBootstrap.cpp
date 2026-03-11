#include "MASkillBootstrap.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/Skill/Runtime/Impl/SK_Broadcast.h"
#include "Agent/Skill/Runtime/Impl/SK_Charge.h"
#include "Agent/Skill/Runtime/Impl/SK_Follow.h"
#include "Agent/Skill/Runtime/Impl/SK_Guide.h"
#include "Agent/Skill/Runtime/Impl/SK_HandleHazard.h"
#include "Agent/Skill/Runtime/Impl/SK_Land.h"
#include "Agent/Skill/Runtime/Impl/SK_Navigate.h"
#include "Agent/Skill/Runtime/Impl/SK_Place.h"
#include "Agent/Skill/Runtime/Impl/SK_ReturnHome.h"
#include "Agent/Skill/Runtime/Impl/SK_Search.h"
#include "Agent/Skill/Runtime/Impl/SK_TakeOff.h"
#include "Agent/Skill/Runtime/Impl/SK_TakePhoto.h"

void FMASkillBootstrap::GrantDefaultAbilities(UMASkillComponent& SkillComponent, AActor* OwnerActor)
{
    if (!OwnerActor)
    {
        return;
    }

    SkillComponent.NavigateSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_Navigate::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.FollowSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_Follow::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.ChargeSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_Charge::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.SearchSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_Search::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.PlaceSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_Place::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.TakeOffSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_TakeOff::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.LandSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_Land::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.ReturnHomeSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_ReturnHome::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.TakePhotoSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_TakePhoto::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.BroadcastSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_Broadcast::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.HandleHazardSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_HandleHazard::StaticClass(), 1, INDEX_NONE, OwnerActor));
    SkillComponent.GuideSkillHandle = SkillComponent.GiveAbility(FGameplayAbilitySpec(USK_Guide::StaticClass(), 1, INDEX_NONE, OwnerActor));
}
