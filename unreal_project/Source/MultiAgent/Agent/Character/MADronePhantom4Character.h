// MADronePhantom4Character.h
// DJI Phantom 4 无人机

#pragma once

#include "CoreMinimal.h"
#include "MADroneCharacter.h"
#include "MADronePhantom4Character.generated.h"

UCLASS()
class MULTIAGENT_API AMADronePhantom4Character : public AMADroneCharacter
{
    GENERATED_BODY()

public:
    AMADronePhantom4Character();

protected:
    virtual void SetupDroneAssets() override;
};
