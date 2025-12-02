#pragma once

#include "CoreMinimal.h"
#include "MAAgent.h"
#include "MARobotDogAgent.generated.h"

UCLASS()
class MULTIAGENT_API AMARobotDogAgent : public AMAAgent
{
    GENERATED_BODY()

public:
    AMARobotDogAgent();

    virtual void Tick(float DeltaTime) override;

    // 播放行走动画
    void PlayWalkAnimation();
    
    // 播放待机动画
    void PlayIdleAnimation();

protected:
    UPROPERTY()
    UAnimSequence* IdleAnim;
    
    UPROPERTY()
    UAnimSequence* WalkAnim;
    
    bool bIsPlayingWalk;
};
