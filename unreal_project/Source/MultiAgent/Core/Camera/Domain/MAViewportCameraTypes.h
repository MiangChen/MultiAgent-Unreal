// MAViewportCameraTypes.h
// 视角上下文的合同类型

#pragma once

#include "CoreMinimal.h"
#include "MAViewportCameraTypes.generated.h"

class AMACharacter;
class UMACameraSensorComponent;
class ACameraActor;

UENUM(BlueprintType)
enum class EMAViewportCameraType : uint8
{
    AgentCamera UMETA(DisplayName = "Agent Camera"),
    ExternalCamera UMETA(DisplayName = "External Camera")
};

USTRUCT()
struct FMAViewportCameraEntry
{
    GENERATED_BODY()

    EMAViewportCameraType Type = EMAViewportCameraType::AgentCamera;

    UPROPERTY()
    TWeakObjectPtr<AMACharacter> Agent;

    UPROPERTY()
    UMACameraSensorComponent* AgentCamera = nullptr;

    UPROPERTY()
    ACameraActor* ExternalCamera = nullptr;

    UPROPERTY()
    AActor* ViewTarget = nullptr;

    FString CameraName;
};
