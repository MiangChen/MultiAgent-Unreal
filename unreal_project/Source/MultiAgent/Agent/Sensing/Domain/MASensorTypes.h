#pragma once

#include "CoreMinimal.h"
#include "MASensorTypes.generated.h"

UENUM(BlueprintType)
enum class EMASensorType : uint8
{
    Camera,
    Lidar,
    Depth,
    IMU
};

UENUM()
enum class EMASensingActionKind : uint8
{
    Invalid,
    TakePhoto,
    StartTCPStream,
    StopTCPStream,
    GetFrameAsJPEG,
};
