// MADroneInspire2Character.h
// DJI Inspire 2 无人机 (边长 3 倍于 Phantom 4)

#pragma once

#include "CoreMinimal.h"
#include "MADroneCharacter.h"
#include "MADroneInspire2Character.generated.h"

UCLASS()
class MULTIAGENT_API AMADroneInspire2Character : public AMADroneCharacter
{
    GENERATED_BODY()

public:
    AMADroneInspire2Character();

protected:
    virtual void SetupDroneAssets() override;
};
