// MAGameMode.cpp

#include "MAGameMode.h"
#include "../Input/MAPlayerController.h"
#include "MAActorSubsystem.h"
#include "MAHumanCharacter.h"
#include "MARobotDogCharacter.h"
#include "MACameraSensor.h"
#include "MAChargingStation.h"
#include "GameFramework/SpectatorPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

AMAGameMode::AMAGameMode()
{
    // 玩家使用 SpectatorPawn 作为上帝视角（原生飞行控制）
    DefaultPawnClass = ASpectatorPawn::StaticClass();
    PlayerControllerClass = AMAPlayerController::StaticClass();
    
    // Character 类型使用 C++ 类
    HumanCharacterClass = AMAHumanCharacter::StaticClass();
    RobotDogCharacterClass = AMARobotDogCharacter::StaticClass();
}

void AMAGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // 延迟一帧生成 Character，确保玩家和子系统已经初始化
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        SpawnInitialCharacters();
    });
}

UMAActorSubsystem* AMAGameMode::GetActorSubsystem() const
{
    return GetWorld()->GetSubsystem<UMAActorSubsystem>();
}

void AMAGameMode::SpawnInitialCharacters()
{
    UMAActorSubsystem* ActorSubsystem = GetActorSubsystem();
    if (!ActorSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ActorSubsystem not found!"));
        return;
    }

    // 使用 PlayerStart 位置作为参考点
    FVector PlayerStart = FVector(0.f, 0.f, 100.f);
    AActor* PlayerStartActor = FindPlayerStart(nullptr);
    if (PlayerStartActor)
    {
        PlayerStart = PlayerStartActor->GetActorLocation();
    }
    
    // 获取导航系统
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    
    int32 TotalCharacters = NumHumans + NumRobotDogs;
    int32 CharacterIndex = 0;
    
    // 生成人类 Character
    for (int32 i = 0; i < NumHumans; i++)
    {
        float Angle = (360.f / TotalCharacters) * CharacterIndex;
        float Radius = 400.f;
        
        FVector SpawnLocation = PlayerStart + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );
        
        // 投影到 NavMesh 上确保位置有效
        if (NavSys)
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(SpawnLocation, NavLocation, FVector(500.f, 500.f, 500.f)))
            {
                SpawnLocation = NavLocation.Location;
            }
        }
        
        AMACharacter* Human = ActorSubsystem->SpawnCharacter(HumanCharacterClass, SpawnLocation, FRotator::ZeroRotator, CharacterIndex, EMAActorType::Human);
        
        // 为 Human 添加第三人称摄像头
        if (Human)
        {
            SpawnAndAttachCamera(ActorSubsystem, Human, FVector(-200.f, 0.f, 100.f), FRotator(-10.f, 0.f, 0.f));
        }
        
        CharacterIndex++;
    }
    
    // 生成机器狗 Character
    for (int32 i = 0; i < NumRobotDogs; i++)
    {
        float Angle = (360.f / TotalCharacters) * CharacterIndex;
        float Radius = 400.f;
        
        FVector SpawnLocation = PlayerStart + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );
        
        // 投影到 NavMesh 上确保位置有效
        if (NavSys)
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(SpawnLocation, NavLocation, FVector(500.f, 500.f, 500.f)))
            {
                SpawnLocation = NavLocation.Location;
            }
        }
        
        AMACharacter* RobotDog = ActorSubsystem->SpawnCharacter(RobotDogCharacterClass, SpawnLocation, FRotator::ZeroRotator, CharacterIndex, EMAActorType::RobotDog);
        
        // 为 RobotDog 添加第三人称摄像头
        if (RobotDog)
        {
            SpawnAndAttachCamera(ActorSubsystem, RobotDog, FVector(-150.f, 0.f, 80.f), FRotator(-15.f, 0.f, 0.f));
        }
        
        CharacterIndex++;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Spawned %d humans and %d robot dogs with cameras via ActorSubsystem"), NumHumans, NumRobotDogs);
    
    // 生成充电站
    SpawnChargingStation();
}

void AMAGameMode::SpawnAndAttachCamera(UMAActorSubsystem* ActorSubsystem, AMACharacter* ParentCharacter, FVector RelativeLocation, FRotator RelativeRotation)
{
    if (!ActorSubsystem || !ParentCharacter)
    {
        return;
    }
    
    // 在父 Character 位置生成摄像头
    AMACameraSensor* Camera = Cast<AMACameraSensor>(
        ActorSubsystem->SpawnSensor(AMACameraSensor::StaticClass(), ParentCharacter->GetActorLocation(), ParentCharacter->GetActorRotation(), -1)
    );
    
    if (Camera)
    {
        // 附着到父 Character
        Camera->AttachToCharacter(ParentCharacter, RelativeLocation, RelativeRotation);
        
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Attached camera %s to %s at offset (%.0f, %.0f, %.0f)"),
            *Camera->SensorName, *ParentCharacter->ActorName, RelativeLocation.X, RelativeLocation.Y, RelativeLocation.Z);
    }
}

void AMAGameMode::SpawnChargingStation()
{
    // 在指定位置生成充电站
    FVector StationLocation = FVector(1000.f, 500.f, 0.f);
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    AMAChargingStation* Station = GetWorld()->SpawnActor<AMAChargingStation>(
        AMAChargingStation::StaticClass(),
        StationLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );
    
    if (Station)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode] Spawned ChargingStation at (%.0f, %.0f, %.0f)"),
            StationLocation.X, StationLocation.Y, StationLocation.Z);
    }
}
