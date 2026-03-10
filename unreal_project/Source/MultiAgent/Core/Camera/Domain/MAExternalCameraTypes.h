// MAExternalCameraTypes.h
// 外部摄像头上下文的合同类型

#pragma once

#include "CoreMinimal.h"
#include "MAExternalCameraTypes.generated.h"

class AMACharacter;
class ACameraActor;
class USpringArmComponent;
class UCameraComponent;

UENUM(BlueprintType)
enum class EMAExternalCameraType : uint8
{
    Fixed UMETA(DisplayName = "Fixed"),
    Follow UMETA(DisplayName = "Follow")
};

UENUM(BlueprintType)
enum class EMAFollowViewType : uint8
{
    Behind UMETA(DisplayName = "Behind"),
    TopDown UMETA(DisplayName = "TopDown"),
    BehindElevated UMETA(DisplayName = "BehindElevated"),
    BehindElevatedReverse UMETA(DisplayName = "BehindElevatedReverse")
};

USTRUCT(BlueprintType)
struct FMAExternalCameraConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CameraName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMAExternalCameraType CameraType = EMAExternalCameraType::Fixed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString TargetAgentLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMAFollowViewType FollowViewType = EMAFollowViewType::Behind;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FollowDistance = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FollowHeight = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseSpringArm = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CameraLagSpeed = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CameraRotationLagSpeed = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FOV = 90.f;
};

USTRUCT()
struct FMAExternalCameraInstance
{
    GENERATED_BODY()

    UPROPERTY()
    FMAExternalCameraConfig Config;

    UPROPERTY()
    ACameraActor* CameraActor = nullptr;

    UPROPERTY()
    AActor* SpringArmCameraActor = nullptr;

    UPROPERTY()
    USpringArmComponent* SpringArmComponent = nullptr;

    UPROPERTY()
    UCameraComponent* CameraComponent = nullptr;

    UPROPERTY()
    TWeakObjectPtr<AMACharacter> TargetAgent;
};
