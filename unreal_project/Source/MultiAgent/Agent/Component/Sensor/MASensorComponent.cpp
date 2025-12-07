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

// ========== Action 动态发现与执行 ==========

TArray<FString> UMASensorComponent::GetAvailableActions() const
{
    // 基类返回空数组，子类应重写
    return TArray<FString>();
}

bool UMASensorComponent::ExecuteAction(const FString& ActionName, const TMap<FString, FString>& Params)
{
    // 基类默认返回 false，子类应重写
    UE_LOG(LogTemp, Warning, TEXT("[Sensor] Action '%s' not implemented in base class"), *ActionName);
    return false;
}
