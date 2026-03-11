#pragma once

#include "CoreMinimal.h"

class UMACameraSensorComponent;

class MULTIAGENT_API FMASensingBootstrap
{
public:
    static void InitializeCameraSensor(UMACameraSensorComponent& CameraSensor);
};
