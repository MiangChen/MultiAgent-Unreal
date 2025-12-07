// MAHumanCharacter.h
// 人类角色

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MAHumanCharacter.generated.h"

UCLASS()
class MULTIAGENT_API AMAHumanCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMAHumanCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void OnNavigationTick() override;
};
