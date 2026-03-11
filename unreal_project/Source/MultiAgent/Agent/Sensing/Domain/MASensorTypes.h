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

struct MULTIAGENT_API FMASensingActionNames
{
    static constexpr const TCHAR* TakePhoto = TEXT("Camera.TakePhoto");
    static constexpr const TCHAR* StartTCPStream = TEXT("Camera.StartTCPStream");
    static constexpr const TCHAR* StopTCPStream = TEXT("Camera.StopTCPStream");
    static constexpr const TCHAR* GetFrameAsJPEG = TEXT("Camera.GetFrameAsJPEG");
};
