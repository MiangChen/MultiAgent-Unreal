// MAAgentManager.cpp

#include "MAAgentManager.h"
#include "../Config/MAConfigManager.h"
#include "../../Agent/Character/MACharacter.h"
#include "../../Agent/Character/MAUAVCharacter.h"
#include "../../Agent/Character/MAFixedWingUAVCharacter.h"
#include "../../Agent/Character/MAUGVCharacter.h"
#include "../../Agent/Character/MAQuadrupedCharacter.h"
#include "../../Agent/Character/MAHumanoidCharacter.h"
#include "../../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "../../Environment/MAChargingStation.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAAgentManager, Log, All);

void UMAAgentManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogMAAgentManager, Log, TEXT("MAAgentManager initialized"));
}

void UMAAgentManager::Deinitialize()
{
    SpawnedAgents.Empty();
    Super::Deinitialize();
}

UMAConfigManager* UMAAgentManager::GetConfigManager() const
{
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        return GI->GetSubsystem<UMAConfigManager>();
    }
    return nullptr;
}

// ========== Agent 生成 ==========

void UMAAgentManager::SpawnAgentsFromConfig()
{
    UMAConfigManager* ConfigMgr = GetConfigManager();
    if (!ConfigMgr || !ConfigMgr->IsConfigLoaded())
    {
        UE_LOG(LogMAAgentManager, Error, TEXT("ConfigManager not available or config not loaded"));
        return;
    }
    
    // 生成充电站
    SpawnChargingStations();
    
    // 生成 Agents
    const TArray<FMAAgentConfigData>& Configs = ConfigMgr->AgentConfigs;
    int32 TotalCount = Configs.Num();
    
    for (int32 i = 0; i < TotalCount; i++)
    {
        const FMAAgentConfigData& Config = Configs[i];
        SpawnAgentInternal(Config.TypeName, Config.ID, Config.Position, Config.Rotation, Config.bAutoPosition, i, TotalCount);
    }
    
    UE_LOG(LogMAAgentManager, Log, TEXT("Spawned %d agents from config"), SpawnedAgents.Num());
}

void UMAAgentManager::SpawnChargingStations()
{
    UMAConfigManager* ConfigMgr = GetConfigManager();
    if (!ConfigMgr) return;
    
    for (const FMAChargingStationConfig& Config : ConfigMgr->ChargingStations)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        
        AMAChargingStation* Station = GetWorld()->SpawnActor<AMAChargingStation>(
            AMAChargingStation::StaticClass(), 
            Config.Position, 
            FRotator::ZeroRotator, 
            SpawnParams
        );
        
        if (Station)
        {
            UE_LOG(LogMAAgentManager, Log, TEXT("Spawned charging station: %s"), *Config.ID);
        }
    }
}

AMACharacter* UMAAgentManager::SpawnAgentByType(const FString& TypeName, FVector Location, FRotator Rotation, bool bAutoPosition)
{
    FString ID = FString::Printf(TEXT("%s_%d"), *TypeName, NextAgentIndex);
    return SpawnAgentInternal(TypeName, ID, Location, Rotation, bAutoPosition, SpawnedAgents.Num(), SpawnedAgents.Num() + 1);
}

AMACharacter* UMAAgentManager::SpawnAgentInternal(const FString& TypeName, const FString& ID, FVector Location, FRotator Rotation, bool bAutoPosition, int32 Index, int32 TotalCount)
{
    FString ClassPath = GetClassPathForType(TypeName);
    if (ClassPath.IsEmpty())
    {
        UE_LOG(LogMAAgentManager, Error, TEXT("Unknown agent type: %s"), *TypeName);
        return nullptr;
    }
    
    UClass* AgentClass = LoadClass<AMACharacter>(nullptr, *ClassPath);
    if (!AgentClass)
    {
        UE_LOG(LogMAAgentManager, Error, TEXT("Failed to load class: %s"), *ClassPath);
        return nullptr;
    }
    
    // 计算生成位置
    FVector SpawnLocation = bAutoPosition ? CalculateAutoPosition(Index, TotalCount) : Location;
    
    // 调整高度
    bool bIsFlying = (TypeName == TEXT("UAV") || TypeName == TEXT("FixedWingUAV"));
    SpawnLocation = AdjustSpawnHeight(SpawnLocation, bIsFlying);
    
    // 生成 Agent
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    AMACharacter* Agent = GetWorld()->SpawnActor<AMACharacter>(AgentClass, SpawnLocation, Rotation, SpawnParams);
    if (Agent)
    {
        Agent->AgentID = ID;
        Agent->AgentName = ID;
        Agent->AgentType = StringToAgentType(TypeName);
        Agent->SpawnDefaultController();
        
        // 添加默认相机
        Agent->AddCameraSensor(FVector(-150.f, 0.f, 80.f), FRotator(-15.f, 0.f, 0.f));
        
        SpawnedAgents.Add(Agent);
        NextAgentIndex++;
        
        OnAgentSpawned.Broadcast(Agent);
        
        UE_LOG(LogMAAgentManager, Log, TEXT("Spawned %s (%s) at (%.0f, %.0f, %.0f)"), 
            *ID, *TypeName, SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);
    }
    
    return Agent;
}

FVector UMAAgentManager::CalculateAutoPosition(int32 Index, int32 TotalCount) const
{
    UMAConfigManager* ConfigMgr = GetConfigManager();
    
    FVector Origin = ConfigMgr ? ConfigMgr->FallbackOrigin : FVector(0, 0, 100);
    float Radius = ConfigMgr ? ConfigMgr->SpawnRadius : 400.f;
    bool bUsePlayerStart = ConfigMgr ? ConfigMgr->bUsePlayerStart : true;
    
    if (bUsePlayerStart)
    {
        if (AActor* PlayerStart = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
        {
            Origin = PlayerStart->GetActorLocation();
        }
    }
    
    if (TotalCount <= 1) return Origin;
    
    float Angle = (360.f / TotalCount) * Index;
    return Origin + FVector(
        FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
        FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
        0.f
    );
}

FVector UMAAgentManager::AdjustSpawnHeight(FVector Location, bool bIsFlying) const
{
    FHitResult HitResult;
    FVector TraceStart = FVector(Location.X, Location.Y, 10000.f);
    FVector TraceEnd = FVector(Location.X, Location.Y, -20000.f);
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
    {
        // 所有机器人都贴地生成，UAV 稍微抬高避免穿模
        float HeightOffset = bIsFlying ? 50.f : 100.f;
        Location.Z = HitResult.Location.Z + HeightOffset;
    }
    
    return Location;
}

// ========== Agent 管理 ==========

bool UMAAgentManager::DestroyAgent(AMACharacter* Agent)
{
    if (!Agent) return false;
    
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

// ========== 辅助函数 ==========

FString UMAAgentManager::GetClassPathForType(const FString& TypeName) const
{
    static TMap<FString, FString> TypeToClass = {
        {TEXT("UAV"), TEXT("/Script/MultiAgent.MAUAVCharacter")},
        {TEXT("FixedWingUAV"), TEXT("/Script/MultiAgent.MAFixedWingUAVCharacter")},
        {TEXT("UGV"), TEXT("/Script/MultiAgent.MAUGVCharacter")},
        {TEXT("Quadruped"), TEXT("/Script/MultiAgent.MAQuadrupedCharacter")},
        {TEXT("Humanoid"), TEXT("/Script/MultiAgent.MAHumanoidCharacter")}
    };
    
    if (const FString* Path = TypeToClass.Find(TypeName))
    {
        return *Path;
    }
    return FString();
}

EMAAgentType UMAAgentManager::StringToAgentType(const FString& TypeString) const
{
    if (TypeString == TEXT("UAV")) return EMAAgentType::UAV;
    if (TypeString == TEXT("FixedWingUAV")) return EMAAgentType::FixedWingUAV;
    if (TypeString == TEXT("UGV")) return EMAAgentType::UGV;
    if (TypeString == TEXT("Quadruped")) return EMAAgentType::Quadruped;
    if (TypeString == TEXT("Humanoid")) return EMAAgentType::Humanoid;
    return EMAAgentType::Humanoid;
}
