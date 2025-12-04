// MARobotDogCharacter.h
// 机器狗角色

#pragma once

#include "CoreMinimal.h"
#include "MACharacter.h"
#include "MARobotDogCharacter.generated.h"

UCLASS()
class MULTIAGENT_API AMARobotDogCharacter : public AMACharacter
{
    GENERATED_BODY()

public:
    AMARobotDogCharacter();

    virtual void Tick(float DeltaTime) override;

    void PlayWalkAnimation();
    void PlayIdleAnimation();

protected:
    UPROPERTY()
    UAnimSequence* IdleAnim;
    
    UPROPERTY()
    UAnimSequence* WalkAnim;
    
    bool bIsPlayingWalk;
};
