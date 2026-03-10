#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAModalModels.h"

class FMAModalCoordinator
{
public:
    FMAModalButtonModel BuildButtonModel(bool bShowEditButton, bool bIsEditMode) const;
    FMAModalAnimationStep StepAnimation(float CurrentTime, float DeltaTime, bool bIsShowing, float Duration) const;
};
