#pragma once

#include "CoreMinimal.h"
#include "../Domain/MAMiniMapModels.h"

class UUserWidget;
class AActor;
class UMAMiniMapWidget;

class FMAMiniMapRuntimeAdapter
{
public:
    FMAMiniMapRuntimeSnapshot Capture(const UUserWidget* Context) const;
    bool MoveViewToWorldLocation(const UUserWidget* Context, const FVector& WorldLocation) const;
};

class FMAMiniMapManagerHUDBridge
{
public:
    UMAMiniMapWidget* ResolveMiniMapWidget(const AActor* Context) const;
};
