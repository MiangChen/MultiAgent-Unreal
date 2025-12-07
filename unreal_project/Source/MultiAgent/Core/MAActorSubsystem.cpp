// MAActorSubsystem.cpp

#include "MAActorSubsystem.h"
#include "MASensor.h"
#include "MARobotDogCharacter.h"
#include "AIController.h"
#include "TimerManager.h"

void UMAActorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMAActorSubsystem::Deinitialize()
{
    StopFormation();
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

// ========== 编队管理 (Formation Manager) ==========

void UMAActorSubsystem::StartFormation(AMACharacter* Leader, EMAFormationType Type)
{
    if (!Leader || Type == EMAFormationType::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] Invalid leader or type"));
        return;
    }
    
    // 停止之前的编队
    StopFormation();
    
    FormationLeader = Leader;
    CurrentFormationType = Type;
    
    // 收集所有 RobotDog 作为编队成员（排除 Tracker）
    FormationMembers.Empty();
    TArray<AMACharacter*> RobotDogs = GetCharactersByType(EMAActorType::RobotDog);
    for (AMACharacter* Robot : RobotDogs)
    {
        if (Robot && !Robot->ActorName.Contains(TEXT("Tracker")))
        {
            FormationMembers.Add(Robot);
        }
    }
    
    if (FormationMembers.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] No robots to form"));
        CurrentFormationType = EMAFormationType::None;
        return;
    }
    
    // 编队名称
    FString FormationName;
    switch (Type)
    {
        case EMAFormationType::Line: FormationName = TEXT("Line"); break;
        case EMAFormationType::Column: FormationName = TEXT("Column"); break;
        case EMAFormationType::Wedge: FormationName = TEXT("Wedge"); break;
        case EMAFormationType::Diamond: FormationName = TEXT("Diamond"); break;
        case EMAFormationType::Circle: FormationName = TEXT("Circle"); break;
        default: FormationName = TEXT("Unknown"); break;
    }
    
    UE_LOG(LogTemp, Log, TEXT("[Formation] Started %s formation with %d robots, Leader: %s"),
        *FormationName, FormationMembers.Num(), *Leader->ActorName);
    
    // 启动定时更新
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FormationTimerHandle,
            this,
            &UMAActorSubsystem::UpdateFormation,
            FormationUpdateInterval,
            true,
            0.f  // 立即执行第一次
        );
    }
}

void UMAActorSubsystem::StopFormation()
{
    if (CurrentFormationType == EMAFormationType::None)
    {
        return;
    }
    
    // 停止定时器
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FormationTimerHandle);
    }
    
    // 停止所有成员的移动
    for (AMACharacter* Member : FormationMembers)
    {
        if (Member)
        {
            Member->CancelNavigation();
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[Formation] Stopped formation"));
    
    FormationLeader.Reset();
    FormationMembers.Empty();
    CurrentFormationType = EMAFormationType::None;
}

void UMAActorSubsystem::SetFormationType(EMAFormationType Type)
{
    if (CurrentFormationType == EMAFormationType::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] Not in formation, call StartFormation first"));
        return;
    }
    
    CurrentFormationType = Type;
    
    // 立即更新一次
    UpdateFormation();
}

void UMAActorSubsystem::UpdateFormation()
{
    // 检查 Leader 是否有效
    if (!FormationLeader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] Leader lost, stopping formation"));
        StopFormation();
        return;
    }
    
    AMACharacter* Leader = FormationLeader.Get();
    FVector LeaderLocation = Leader->GetActorLocation();
    FRotator LeaderRotation = Leader->GetActorRotation();
    
    // 计算所有编队位置
    TArray<FVector> Positions = CalculateFormationPositions(LeaderLocation, LeaderRotation);
    
    // 下发导航指令（使用滞后区避免抖动）
    for (int32 i = 0; i < FormationMembers.Num() && i < Positions.Num(); i++)
    {
        AMACharacter* Member = FormationMembers[i];
        if (!Member) continue;
        
        // 检查机器人是否有能量
        if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Member))
        {
            if (!Robot->HasEnergy())
            {
                continue;  // 没电的机器人跳过
            }
        }
        
        FVector CurrentLocation = Member->GetActorLocation();
        float DistanceToTarget = FVector::Dist2D(CurrentLocation, Positions[i]);
        
        // 滞后区逻辑：
        // - 如果正在移动，距离 < StopThreshold 才停止
        // - 如果已停止，距离 > StartThreshold 才开始移动
        if (Member->bIsMoving)
        {
            if (DistanceToTarget < FormationStopMoveThreshold)
            {
                // 到达目标，停止移动
                Member->CancelNavigation();
            }
            // 否则继续当前移动，不重新发起请求
        }
        else
        {
            if (DistanceToTarget > FormationStartMoveThreshold)
            {
                // 距离较远，开始移动
                Member->TryNavigateTo(Positions[i]);
            }
            // 否则保持静止
        }
    }
}

TArray<FVector> UMAActorSubsystem::CalculateFormationPositions(const FVector& LeaderLocation, const FRotator& LeaderRotation) const
{
    TArray<FVector> Positions;
    int32 TotalCount = FormationMembers.Num();
    
    for (int32 i = 0; i < TotalCount; i++)
    {
        FVector LocalOffset = CalculateFormationOffset(i, TotalCount);
        FVector WorldOffset = LeaderRotation.RotateVector(LocalOffset);
        Positions.Add(LeaderLocation + WorldOffset);
    }
    
    return Positions;
}

FVector UMAActorSubsystem::CalculateFormationOffset(int32 Position, int32 TotalCount) const
{
    FVector Offset = FVector::ZeroVector;
    
    switch (CurrentFormationType)
    {
        case EMAFormationType::Line:
            // 横向一字排开：左右交替
            if (Position % 2 == 0)
                Offset = FVector(0.f, FormationSpacing * ((Position / 2) + 1), 0.f);
            else
                Offset = FVector(0.f, -FormationSpacing * ((Position / 2) + 1), 0.f);
            break;
            
        case EMAFormationType::Column:
            // 纵队：依次排在后面
            Offset = FVector(-FormationSpacing * (Position + 1), 0.f, 0.f);
            break;
            
        case EMAFormationType::Wedge:
            // V形楔形：左右交替，逐渐向后
            {
                int32 Row = (Position / 2) + 1;
                float Side = (Position % 2 == 0) ? 1.f : -1.f;
                Offset = FVector(-FormationSpacing * Row, FormationSpacing * Row * Side * 0.7f, 0.f);
            }
            break;
            
        case EMAFormationType::Diamond:
            // 菱形：四个方向
            switch (Position % 4)
            {
                case 0: Offset = FVector(-FormationSpacing, 0.f, 0.f); break;  // 后
                case 1: Offset = FVector(0.f, FormationSpacing, 0.f); break;   // 右
                case 2: Offset = FVector(0.f, -FormationSpacing, 0.f); break;  // 左
                case 3: Offset = FVector(FormationSpacing, 0.f, 0.f); break;   // 前
            }
            if (Position >= 4)
            {
                Offset *= (Position / 4) + 1;
            }
            break;
            
        case EMAFormationType::Circle:
            // 圆形：根据数量动态计算半径
            {
                float MinSpacing = FormationSpacing * 0.8f;
                float Radius = FMath::Max(FormationSpacing, (TotalCount * MinSpacing) / (2.f * PI));
                float Angle = (static_cast<float>(Position) / static_cast<float>(TotalCount)) * 2.f * PI;
                Offset = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
            }
            break;
            
        default:
            break;
    }
    
    return Offset;
}
