#pragma once

#include "CoreMinimal.h"
#include "../../MACommTypes.h"

namespace MACommTypeHelpers
{
    FString MessageTypeToString(EMACommMessageType Type);
    EMACommMessageType StringToMessageType(const FString& TypeStr);

    FString MessageCategoryToString(EMAMessageCategory Category);
    EMAMessageCategory StringToMessageCategory(const FString& CategoryStr);

    FString MessageDirectionToString(EMAMessageDirection Direction);
    EMAMessageDirection StringToMessageDirection(const FString& DirectionStr);

    EMAMessageCategory GetCategoryForMessageType(EMACommMessageType Type);
}
