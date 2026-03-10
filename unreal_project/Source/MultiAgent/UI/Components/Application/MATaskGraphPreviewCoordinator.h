#pragma once

#include "CoreMinimal.h"
#include "UI/Components/Domain/MATaskGraphPreviewModels.h"

class FMATaskGraphPreviewCoordinator
{
public:
    static FMATaskGraphPreviewModel BuildModel(const FMATaskGraphData& Data);
    static FMATaskGraphPreviewModel MakeEmptyModel();
};
