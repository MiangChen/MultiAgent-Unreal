// MASensorComponent.cpp

#include "MASensorComponent.h"

UMASensorComponent::UMASensorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    
    SensorID = 0;
    SensorName = TEXT("Sensor");
    SensorType = EMASensorType::Camera;
}

void UMASensorComponent::BeginPlay()
{
    Super::BeginPlay();
}
