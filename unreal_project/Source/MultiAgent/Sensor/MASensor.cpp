// MASensor.cpp

#include "MASensor.h"
#include "MACharacter.h"

AMASensor::AMASensor()
{
    PrimaryActorTick.bCanEverTick = false;
    
    SensorID = 0;
    SensorName = TEXT("Sensor");
    SensorType = EMASensorType::Camera;
    
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    SetRootComponent(RootSceneComponent);
}

void AMASensor::BeginPlay()
{
    Super::BeginPlay();
}

void AMASensor::AttachToCharacter(AMACharacter* ParentCharacter, FVector RelativeLocation, FRotator RelativeRotation)
{
    if (!ParentCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Sensor] AttachToCharacter failed: ParentCharacter is null"));
        return;
    }
    
    AttachedToCharacter = ParentCharacter;
    
    FAttachmentTransformRules Rules(
        EAttachmentRule::KeepRelative,
        EAttachmentRule::KeepRelative,
        EAttachmentRule::KeepWorld,
        true
    );
    
    AttachToActor(ParentCharacter, Rules);
    SetActorRelativeLocation(RelativeLocation);
    SetActorRelativeRotation(RelativeRotation);
    
    UE_LOG(LogTemp, Log, TEXT("[Sensor] %s attached to %s at (%.0f, %.0f, %.0f)"),
        *SensorName, *ParentCharacter->ActorName,
        RelativeLocation.X, RelativeLocation.Y, RelativeLocation.Z);
}

void AMASensor::DetachFromCharacter()
{
    if (!AttachedToCharacter.IsValid())
    {
        return;
    }
    
    FString ParentName = AttachedToCharacter->ActorName;
    
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    AttachedToCharacter.Reset();
    
    UE_LOG(LogTemp, Log, TEXT("[Sensor] %s detached from %s"), *SensorName, *ParentName);
}

AMACharacter* AMASensor::GetAttachedCharacter() const
{
    return AttachedToCharacter.Get();
}

bool AMASensor::IsAttached() const
{
    return AttachedToCharacter.IsValid();
}
