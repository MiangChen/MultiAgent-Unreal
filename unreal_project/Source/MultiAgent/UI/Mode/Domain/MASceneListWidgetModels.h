#pragma once

#include "CoreMinimal.h"

struct FMASceneListItemModel
{
    FString Id;
    FString Label;
};

struct FMASceneListSectionModel
{
    FString CountText = TEXT("(0)");
    TArray<FMASceneListItemModel> Items;
    FString EmptyText;
};
