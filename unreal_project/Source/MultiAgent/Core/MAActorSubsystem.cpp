// MAActorSubsystem.cpp

#include "MAActorSubsystem.h"
#include "../Actor/MASensor.h"
#include "AIController.h"

void UMAActorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMAActorSubsystem::Deinitialize()
{
    SpawnedCharacters.Empty();
    SpawnedSensors.Empty();
    Super::Deinitialize();
}

// ========== Character 管理 ==========

AMACharacter* UMAActorSubsystem::SpawnCharacter(TSubclassOf<AMACharacter> CharacterClass, FVector Location, FRotator Rotation, int32 ActorID, EMAActorType Type)
{
    if (!CharacterClass)
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AMACharacter* NewCharacter = World->SpawnActor<AMACharacter>(CharacterClass, Location, Rotation, SpawnParams);
    if (NewCharacter)
    {
        NewCharacter->ActorID = ActorID >= 0 ? ActorID : NextActorID++;
        NewCharacter->ActorType = Type;
        
        const TCHAR* TypeName = TEXT("Character");
        switch (Type)
        {
            case EMAActorType::Human: TypeName = TEXT("Human"); break;
            case EMAActorType::RobotDog: TypeName = TEXT("RobotDog"); break;
            case EMAActorType::Drone: TypeName = TEXT("Drone"); break;
            case EMAActorType::Camera: TypeName = TEXT("Camera"); break;
        }
        NewCharacter->ActorName = FString::Printf(TEXT("%s_%d"), TypeName, NewCharacter->ActorID);

        NewCharacter->SpawnDefaultController();
        SpawnedCharacters.Add(NewCharacter);

        OnCharacterSpawned.Broadcast(NewCharacter);
    }

    return NewCharacter;
}

bool UMAActorSubsystem::DestroyCharacter(AMACharacter* Character)
{
    if (!Character)
    {
        return false;
    }

    if (SpawnedCharacters.Remove(Character) > 0)
    {
        OnCharacterDestroyed.Broadcast(Character);
        Character->Destroy();
        return true;
    }

    return false;
}

TArray<AMACharacter*> UMAActorSubsystem::GetCharactersByType(EMAActorType Type) const
{
    TArray<AMACharacter*> Result;
    for (AMACharacter* Character : SpawnedCharacters)
    {
        if (Character && Character->ActorType == Type)
        {
            Result.Add(Character);
        }
    }
    return Result;
}

AMACharacter* UMAActorSubsystem::GetCharacterByID(int32 ActorID) const
{
    for (AMACharacter* Character : SpawnedCharacters)
    {
        if (Character && Character->ActorID == ActorID)
        {
            return Character;
        }
    }
    return nullptr;
}

AMACharacter* UMAActorSubsystem::GetCharacterByName(const FString& Name) const
{
    for (AMACharacter* Character : SpawnedCharacters)
    {
        if (Character && Character->ActorName == Name)
        {
            return Character;
        }
    }
    return nullptr;
}

// ========== Sensor 管理 ==========

AMASensor* UMAActorSubsystem::SpawnSensor(TSubclassOf<AMASensor> SensorClass, FVector Location, FRotator Rotation, int32 SensorID)
{
    if (!SensorClass)
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AMASensor* NewSensor = World->SpawnActor<AMASensor>(SensorClass, Location, Rotation, SpawnParams);
    if (NewSensor)
    {
        NewSensor->SensorID = SensorID >= 0 ? SensorID : NextSensorID++;
        NewSensor->SensorName = FString::Printf(TEXT("Sensor_%d"), NewSensor->SensorID);

        SpawnedSensors.Add(NewSensor);
        OnSensorSpawned.Broadcast(NewSensor);
    }

    return NewSensor;
}

bool UMAActorSubsystem::DestroySensor(AMASensor* Sensor)
{
    if (!Sensor)
    {
        return false;
    }

    if (SpawnedSensors.Remove(Sensor) > 0)
    {
        OnSensorDestroyed.Broadcast(Sensor);
        Sensor->Destroy();
        return true;
    }

    return false;
}

// ========== 批量操作 ==========

void UMAActorSubsystem::StopAllCharacters()
{
    for (AMACharacter* Character : SpawnedCharacters)
    {
        if (Character)
        {
            Character->CancelNavigation();
        }
    }
}

void UMAActorSubsystem::MoveAllCharactersTo(FVector Destination, float Radius)
{
    int32 Count = SpawnedCharacters.Num();
    if (Count == 0) return;

    for (int32 i = 0; i < Count; i++)
    {
        AMACharacter* Character = SpawnedCharacters[i];
        if (!Character) continue;

        float Angle = (360.f / Count) * i;
        FVector TargetLocation = Destination + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );

        Character->TryNavigateTo(TargetLocation);
    }
}
