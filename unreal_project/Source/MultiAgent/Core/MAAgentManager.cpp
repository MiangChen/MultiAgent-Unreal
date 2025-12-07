// MAAgentManager.cpp

#include "MAAgentManager.h"
#include "../Agent/Character/MARobotDogCharacter.h"
#include "../Environment/MAChargingStation.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "../Agent/Character/MACharacter.h"
#include "AIController.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"

void UMAAgentManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UMAAgentManager::Deinitialize()
{
    StopFormation();
    SpawnedAgents.Empty();
    Super::Deinitialize();
}

// ========== 配置加载 ==========

bool UMAAgentManager::LoadAndSpawnFromConfig(const FString& ConfigPath)
{
    if (!LoadConfig(ConfigPath))
    {
        return false;
    }
    
    SpawnAllAgentsFromConfig();
    return true;
}

bool UMAAgentManager::LoadConfig(const FString& ConfigPath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[AgentManager] Failed to load config file: %s"), *ConfigPath);
        return false;
    }
    
    if (!ParseConfigJson(JsonString))
    {
        UE_LOG(LogTemp, Error, TEXT("[AgentManager] Failed to parse config JSON"));
        return false;
    }
    
    bConfigLoaded = true;
    UE_LOG(LogTemp, Log, TEXT("[AgentManager] Loaded config with %d agents"), AgentConfigs.Num());
    return true;
}

bool UMAAgentManager::ParseConfigJson(const FString& JsonString)
{
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        return false;
    }
    
    // 解析 spawn_settings
    if (const TSharedPtr<FJsonObject>* SpawnSettingsObj; RootObject->TryGetObjectField(TEXT("spawn_settings"), SpawnSettingsObj))
    {
        (*SpawnSettingsObj)->TryGetBoolField(TEXT("use_player_start"), SpawnSettings.bUsePlayerStart);
        (*SpawnSettingsObj)->TryGetBoolField(TEXT("project_to_navmesh"), SpawnSettings.bProjectToNavMesh);
        (*SpawnSettingsObj)->TryGetNumberField(TEXT("spawn_radius"), SpawnSettings.SpawnRadius);
        
        if (const TArray<TSharedPtr<FJsonValue>>* OriginArray; (*SpawnSettingsObj)->TryGetArrayField(TEXT("fallback_origin"), OriginArray))
        {
            if (OriginArray->Num() >= 3)
            {
                SpawnSettings.FallbackOrigin = FVector(
                    (*OriginArray)[0]->AsNumber(),
                    (*OriginArray)[1]->AsNumber(),
                    (*OriginArray)[2]->AsNumber()
                );
            }
        }
    }
    
    // 解析 agents
    AgentConfigs.Empty();
    if (const TArray<TSharedPtr<FJsonValue>>* AgentsArray; RootObject->TryGetArrayField(TEXT("agents"), AgentsArray))
    {
        for (const TSharedPtr<FJsonValue>& AgentValue : *AgentsArray)
        {
            const TSharedPtr<FJsonObject>& AgentObj = AgentValue->AsObject();
            if (!AgentObj.IsValid()) continue;
            
            FMAAgentConfig Config;
            AgentObj->TryGetStringField(TEXT("id"), Config.ID);
            
            FString TypeString;
            if (AgentObj->TryGetStringField(TEXT("type"), TypeString))
            {
                Config.Type = StringToAgentType(TypeString);
            }
            
            AgentObj->TryGetStringField(TEXT("class"), Config.ClassName);
            
            // 解析 position
            if (FString PosString; AgentObj->TryGetStringField(TEXT("position"), PosString))
            {
                Config.bAutoPosition = (PosString == TEXT("auto"));
            }
            else if (const TArray<TSharedPtr<FJsonValue>>* PosArray; AgentObj->TryGetArrayField(TEXT("position"), PosArray))
            {
                if (PosArray->Num() >= 3)
                {
                    Config.Position = FVector(
                        (*PosArray)[0]->AsNumber(),
                        (*PosArray)[1]->AsNumber(),
                        (*PosArray)[2]->AsNumber()
                    );
                    Config.bAutoPosition = false;
                }
            }
            
            // 解析 rotation
            if (const TArray<TSharedPtr<FJsonValue>>* RotArray; AgentObj->TryGetArrayField(TEXT("rotation"), RotArray))
            {
                if (RotArray->Num() >= 3)
                {
                    Config.Rotation = FRotator(
                        (*RotArray)[0]->AsNumber(),
                        (*RotArray)[1]->AsNumber(),
                        (*RotArray)[2]->AsNumber()
                    );
                }
            }
            
            // 解析 sensors
            if (const TArray<TSharedPtr<FJsonValue>>* SensorsArray; AgentObj->TryGetArrayField(TEXT("sensors"), SensorsArray))
            {
                for (const TSharedPtr<FJsonValue>& SensorValue : *SensorsArray)
                {
                    const TSharedPtr<FJsonObject>& SensorObj = SensorValue->AsObject();
                    if (!SensorObj.IsValid()) continue;
                    
                    FMASensorConfig SensorConfig;
                    SensorObj->TryGetStringField(TEXT("type"), SensorConfig.Type);
                    
                    if (const TArray<TSharedPtr<FJsonValue>>* SPosArray; SensorObj->TryGetArrayField(TEXT("position"), SPosArray))
                    {
                        if (SPosArray->Num() >= 3)
                        {
                            SensorConfig.Position = FVector(
                                (*SPosArray)[0]->AsNumber(),
                                (*SPosArray)[1]->AsNumber(),
                                (*SPosArray)[2]->AsNumber()
                            );
                        }
                    }
                    
                    if (const TArray<TSharedPtr<FJsonValue>>* SRotArray; SensorObj->TryGetArrayField(TEXT("rotation"), SRotArray))
                    {
                        if (SRotArray->Num() >= 3)
                        {
                            SensorConfig.Rotation = FRotator(
                                (*SRotArray)[0]->AsNumber(),
                                (*SRotArray)[1]->AsNumber(),
                                (*SRotArray)[2]->AsNumber()
                            );
                        }
                    }
                    
                    // 解析 params
                    if (const TSharedPtr<FJsonObject>* ParamsObj; SensorObj->TryGetObjectField(TEXT("params"), ParamsObj))
                    {
                        if (const TArray<TSharedPtr<FJsonValue>>* ResArray; (*ParamsObj)->TryGetArrayField(TEXT("resolution"), ResArray))
                        {
                            if (ResArray->Num() >= 2)
                            {
                                SensorConfig.Resolution = FIntPoint(
                                    static_cast<int32>((*ResArray)[0]->AsNumber()),
                                    static_cast<int32>((*ResArray)[1]->AsNumber())
                                );
                            }
                        }
                        (*ParamsObj)->TryGetNumberField(TEXT("fov"), SensorConfig.FOV);
                    }
                    
                    Config.Sensors.Add(SensorConfig);
                }
            }
            
            AgentConfigs.Add(Config);
        }
    }
    
    // 解析 environment.charging_stations
    ChargingStationConfigs.Empty();
    if (const TSharedPtr<FJsonObject>* EnvObj; RootObject->TryGetObjectField(TEXT("environment"), EnvObj))
    {
        if (const TArray<TSharedPtr<FJsonValue>>* StationsArray; (*EnvObj)->TryGetArrayField(TEXT("charging_stations"), StationsArray))
        {
            for (const TSharedPtr<FJsonValue>& StationValue : *StationsArray)
            {
                const TSharedPtr<FJsonObject>& StationObj = StationValue->AsObject();
                if (!StationObj.IsValid()) continue;
                
                FMAChargingStationConfig StationConfig;
                StationObj->TryGetStringField(TEXT("id"), StationConfig.ID);
                
                if (const TArray<TSharedPtr<FJsonValue>>* PosArray; StationObj->TryGetArrayField(TEXT("position"), PosArray))
                {
                    if (PosArray->Num() >= 3)
                    {
                        StationConfig.Position = FVector(
                            (*PosArray)[0]->AsNumber(),
                            (*PosArray)[1]->AsNumber(),
                            (*PosArray)[2]->AsNumber()
                        );
                    }
                }
                
                ChargingStationConfigs.Add(StationConfig);
            }
        }
    }
    
    return true;
}

void UMAAgentManager::SpawnAllAgentsFromConfig()
{
    if (!bConfigLoaded)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AgentManager] No config loaded, call LoadConfig first"));
        return;
    }
    
    int32 TotalCount = AgentConfigs.Num();
    for (int32 i = 0; i < TotalCount; i++)
    {
        SpawnAgentFromConfig(AgentConfigs[i], i, TotalCount);
    }
    
    UE_LOG(LogTemp, Log, TEXT("[AgentManager] Spawned %d agents from config"), SpawnedAgents.Num());
    
    // 生成充电站
    SpawnChargingStations();
}

AMACharacter* UMAAgentManager::SpawnAgentFromConfig(const FMAAgentConfig& Config, int32 Index, int32 TotalCount)
{
    // 加载类
    UClass* AgentClass = LoadClass<AMACharacter>(nullptr, *Config.ClassName);
    if (!AgentClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[AgentManager] Failed to load class: %s"), *Config.ClassName);
        return nullptr;
    }
    
    // 计算位置
    FVector SpawnLocation = Config.bAutoPosition ? CalculateAutoPosition(Index, TotalCount) : Config.Position;
    
    // 投影到 NavMesh
    if (SpawnSettings.bProjectToNavMesh)
    {
        if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
        {
            FNavLocation NavLocation;
            if (NavSys->ProjectPointToNavigation(SpawnLocation, NavLocation, FVector(500.f, 500.f, 500.f)))
            {
                SpawnLocation = NavLocation.Location;
            }
        }
    }
    
    // 生成 Agent
    AMACharacter* Agent = SpawnAgent(AgentClass, SpawnLocation, Config.Rotation, Config.ID, Config.Type);
    
    // 添加 Sensors
    if (Agent)
    {
        AddSensorsToAgent(Agent, Config.Sensors);
    }
    
    return Agent;
}

void UMAAgentManager::AddSensorsToAgent(AMACharacter* Agent, const TArray<FMASensorConfig>& SensorConfigs)
{
    if (!Agent) return;
    
    for (const FMASensorConfig& SensorConfig : SensorConfigs)
    {
        if (SensorConfig.Type == TEXT("Camera"))
        {
            UMACameraSensorComponent* Camera = Agent->AddCameraSensor(SensorConfig.Position, SensorConfig.Rotation);
            if (Camera)
            {
                Camera->Resolution = SensorConfig.Resolution;
                Camera->FOV = SensorConfig.FOV;
            }
        }
        // 未来可以扩展其他 Sensor 类型
        // else if (SensorConfig.Type == TEXT("Lidar")) { ... }
    }
}

FVector UMAAgentManager::CalculateAutoPosition(int32 Index, int32 TotalCount) const
{
    // 获取基准位置
    FVector Origin = SpawnSettings.FallbackOrigin;
    
    if (SpawnSettings.bUsePlayerStart)
    {
        if (UWorld* World = GetWorld())
        {
            if (AActor* PlayerStart = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass()))
            {
                Origin = PlayerStart->GetActorLocation();
            }
        }
    }
    
    // 圆形分布
    float Angle = (360.f / TotalCount) * Index;
    return Origin + FVector(
        FMath::Cos(FMath::DegreesToRadians(Angle)) * SpawnSettings.SpawnRadius,
        FMath::Sin(FMath::DegreesToRadians(Angle)) * SpawnSettings.SpawnRadius,
        0.f
    );
}

void UMAAgentManager::SpawnChargingStations()
{
    UWorld* World = GetWorld();
    if (!World) return;
    
    for (const FMAChargingStationConfig& Config : ChargingStationConfigs)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        
        AMAChargingStation* Station = World->SpawnActor<AMAChargingStation>(
            AMAChargingStation::StaticClass(),
            Config.Position,
            FRotator::ZeroRotator,
            SpawnParams
        );
        
        if (Station)
        {
            UE_LOG(LogTemp, Log, TEXT("[AgentManager] Spawned ChargingStation '%s' at (%.0f, %.0f, %.0f)"),
                *Config.ID, Config.Position.X, Config.Position.Y, Config.Position.Z);
        }
    }
}

// ========== Agent 管理 ==========

AMACharacter* UMAAgentManager::SpawnAgent(TSubclassOf<AMACharacter> AgentClass, FVector Location, FRotator Rotation, const FString& AgentID, EMAAgentType Type)
{
    if (!AgentClass)
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

    AMACharacter* NewAgent = World->SpawnActor<AMACharacter>(AgentClass, Location, Rotation, SpawnParams);
    if (NewAgent)
    {
        NewAgent->AgentID = AgentID.IsEmpty() ? FString::Printf(TEXT("Agent_%d"), NextAgentIndex) : AgentID;
        NewAgent->AgentType = Type;
        NewAgent->AgentName = NewAgent->AgentID;  // 默认 Name = ID

        NewAgent->SpawnDefaultController();
        SpawnedAgents.Add(NewAgent);
        NextAgentIndex++;

        OnAgentSpawned.Broadcast(NewAgent);
        
        UE_LOG(LogTemp, Log, TEXT("[AgentManager] Spawned Agent: %s (Type: %s)"), 
            *NewAgent->AgentID, *AgentTypeToString(Type));
    }

    return NewAgent;
}

bool UMAAgentManager::DestroyAgent(AMACharacter* Agent)
{
    if (!Agent)
    {
        return false;
    }

    if (SpawnedAgents.Remove(Agent) > 0)
    {
        OnAgentDestroyed.Broadcast(Agent);
        Agent->Destroy();
        return true;
    }

    return false;
}

TArray<AMACharacter*> UMAAgentManager::GetAgentsByType(EMAAgentType Type) const
{
    TArray<AMACharacter*> Result;
    for (AMACharacter* Agent : SpawnedAgents)
    {
        if (Agent && Agent->AgentType == Type)
        {
            Result.Add(Agent);
        }
    }
    return Result;
}

AMACharacter* UMAAgentManager::GetAgentByID(const FString& AgentID) const
{
    for (AMACharacter* Agent : SpawnedAgents)
    {
        if (Agent && Agent->AgentID == AgentID)
        {
            return Agent;
        }
    }
    return nullptr;
}

AMACharacter* UMAAgentManager::GetAgentByIndex(int32 Index) const
{
    if (Index >= 0 && Index < SpawnedAgents.Num())
    {
        return SpawnedAgents[Index];
    }
    return nullptr;
}

// ========== Action 动态发现 ==========

TArray<FString> UMAAgentManager::GetAgentAvailableActions(AMACharacter* Agent) const
{
    TArray<FString> Actions;
    if (!Agent) return Actions;
    
    // 获取 Agent 自身的 Actions
    Actions.Append(Agent->GetAvailableActions());
    
    return Actions;
}

bool UMAAgentManager::ExecuteAgentAction(AMACharacter* Agent, const FString& ActionName, const TMap<FString, FString>& Params)
{
    if (!Agent) return false;
    
    return Agent->ExecuteAction(ActionName, Params);
}

TMap<FString, TArray<FString>> UMAAgentManager::GetAllAgentsActions() const
{
    TMap<FString, TArray<FString>> Result;
    
    for (AMACharacter* Agent : SpawnedAgents)
    {
        if (Agent)
        {
            Result.Add(Agent->AgentID, GetAgentAvailableActions(Agent));
        }
    }
    
    return Result;
}

// ========== 批量操作 ==========

void UMAAgentManager::StopAllAgents()
{
    for (AMACharacter* Agent : SpawnedAgents)
    {
        if (Agent)
        {
            Agent->CancelNavigation();
        }
    }
}

void UMAAgentManager::MoveAllAgentsTo(FVector Destination, float Radius)
{
    int32 Count = SpawnedAgents.Num();
    if (Count == 0) return;

    for (int32 i = 0; i < Count; i++)
    {
        AMACharacter* Agent = SpawnedAgents[i];
        if (!Agent) continue;

        float Angle = (360.f / Count) * i;
        FVector TargetLocation = Destination + FVector(
            FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
            FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
            0.f
        );

        Agent->TryNavigateTo(TargetLocation);
    }
}

// ========== 编队管理 ==========

void UMAAgentManager::StartFormation(AMACharacter* Leader, EMAFormationType Type)
{
    if (!Leader || Type == EMAFormationType::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] Invalid leader or type"));
        return;
    }
    
    StopFormation();
    
    FormationLeader = Leader;
    CurrentFormationType = Type;
    
    // 收集所有 RobotDog 作为编队成员（排除 Tracker）
    FormationMembers.Empty();
    TArray<AMACharacter*> RobotDogs = GetAgentsByType(EMAAgentType::RobotDog);
    for (AMACharacter* Robot : RobotDogs)
    {
        if (Robot && !Robot->AgentName.Contains(TEXT("Tracker")))
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
        *FormationName, FormationMembers.Num(), *Leader->AgentName);
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            FormationTimerHandle,
            this,
            &UMAAgentManager::UpdateFormation,
            FormationUpdateInterval,
            true,
            0.f
        );
    }
}

void UMAAgentManager::StopFormation()
{
    if (CurrentFormationType == EMAFormationType::None)
    {
        return;
    }
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FormationTimerHandle);
    }
    
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

void UMAAgentManager::SetFormationType(EMAFormationType Type)
{
    if (CurrentFormationType == EMAFormationType::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] Not in formation, call StartFormation first"));
        return;
    }
    
    CurrentFormationType = Type;
    UpdateFormation();
}

void UMAAgentManager::UpdateFormation()
{
    if (!FormationLeader.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Formation] Leader lost, stopping formation"));
        StopFormation();
        return;
    }
    
    AMACharacter* Leader = FormationLeader.Get();
    FVector LeaderLocation = Leader->GetActorLocation();
    FRotator LeaderRotation = Leader->GetActorRotation();
    
    TArray<FVector> Positions = CalculateFormationPositions(LeaderLocation, LeaderRotation);
    
    for (int32 i = 0; i < FormationMembers.Num() && i < Positions.Num(); i++)
    {
        AMACharacter* Member = FormationMembers[i];
        if (!Member) continue;
        
        if (AMARobotDogCharacter* Robot = Cast<AMARobotDogCharacter>(Member))
        {
            if (!Robot->HasEnergy())
            {
                continue;
            }
        }
        
        FVector CurrentLocation = Member->GetActorLocation();
        float DistanceToTarget = FVector::Dist2D(CurrentLocation, Positions[i]);
        
        if (Member->bIsMoving)
        {
            if (DistanceToTarget < FormationStopMoveThreshold)
            {
                Member->CancelNavigation();
            }
        }
        else
        {
            if (DistanceToTarget > FormationStartMoveThreshold)
            {
                Member->TryNavigateTo(Positions[i]);
            }
        }
    }
}

TArray<FVector> UMAAgentManager::CalculateFormationPositions(const FVector& LeaderLocation, const FRotator& LeaderRotation) const
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

FVector UMAAgentManager::CalculateFormationOffset(int32 Position, int32 TotalCount) const
{
    FVector Offset = FVector::ZeroVector;
    
    switch (CurrentFormationType)
    {
        case EMAFormationType::Line:
            if (Position % 2 == 0)
                Offset = FVector(0.f, FormationSpacing * ((Position / 2) + 1), 0.f);
            else
                Offset = FVector(0.f, -FormationSpacing * ((Position / 2) + 1), 0.f);
            break;
            
        case EMAFormationType::Column:
            Offset = FVector(-FormationSpacing * (Position + 1), 0.f, 0.f);
            break;
            
        case EMAFormationType::Wedge:
            {
                int32 Row = (Position / 2) + 1;
                float Side = (Position % 2 == 0) ? 1.f : -1.f;
                Offset = FVector(-FormationSpacing * Row, FormationSpacing * Row * Side * 0.7f, 0.f);
            }
            break;
            
        case EMAFormationType::Diamond:
            switch (Position % 4)
            {
                case 0: Offset = FVector(-FormationSpacing, 0.f, 0.f); break;
                case 1: Offset = FVector(0.f, FormationSpacing, 0.f); break;
                case 2: Offset = FVector(0.f, -FormationSpacing, 0.f); break;
                case 3: Offset = FVector(FormationSpacing, 0.f, 0.f); break;
            }
            if (Position >= 4)
            {
                Offset *= (Position / 4) + 1;
            }
            break;
            
        case EMAFormationType::Circle:
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

// ========== 辅助函数 ==========

EMAAgentType UMAAgentManager::StringToAgentType(const FString& TypeString) const
{
    if (TypeString == TEXT("Human")) return EMAAgentType::Human;
    if (TypeString == TEXT("RobotDog")) return EMAAgentType::RobotDog;
    if (TypeString == TEXT("Drone")) return EMAAgentType::Drone;
    if (TypeString == TEXT("Camera")) return EMAAgentType::Camera;
    return EMAAgentType::Human;
}

FString UMAAgentManager::AgentTypeToString(EMAAgentType Type) const
{
    switch (Type)
    {
        case EMAAgentType::Human: return TEXT("Human");
        case EMAAgentType::RobotDog: return TEXT("RobotDog");
        case EMAAgentType::Drone: return TEXT("Drone");
        case EMAAgentType::Camera: return TEXT("Camera");
        default: return TEXT("Unknown");
    }
}
