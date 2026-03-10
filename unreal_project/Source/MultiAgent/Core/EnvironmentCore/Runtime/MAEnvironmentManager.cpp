// MAEnvironmentManager.cpp
// 环境实体管理器实现

#include "MAEnvironmentManager.h"
#include "Core/SceneGraph/Runtime/MASceneGraphManager.h"
#include "Core/SceneGraph/Bootstrap/MASceneGraphBootstrap.h"
#include "Core/Config/MAConfigManager.h"
#include "Environment/Entity/MAChargingStation.h"
#include "Environment/Entity/MACargo.h"
#include "Environment/Entity/MAPerson.h"
#include "Environment/Entity/MAVehicle.h"
#include "Environment/Entity/MABoat.h"
#include "Environment/Effect/MAFire.h"
#include "Environment/Effect/MASmoke.h"
#include "Environment/Effect/MAWind.h"
#include "Environment/Entity/MAComponent.h"
#include "Environment/IMAEnvironmentObject.h"

DEFINE_LOG_CATEGORY(LogMAEnvironmentManager);

void UMAEnvironmentManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogMAEnvironmentManager, Log, TEXT("MAEnvironmentManager initialized"));
}

void UMAEnvironmentManager::Deinitialize()
{
    SpawnedObjects.Empty();
    SpawnedPersons.Empty();
    SpawnedVehicles.Empty();
    SpawnedBoats.Empty();
    SpawnedPickupItems.Empty();
    SpawnedChargingStations.Empty();
    SpawnedComponents.Empty();
    SpawnedEffects.Empty();
    Super::Deinitialize();
}

UMAConfigManager* UMAEnvironmentManager::GetConfigManager() const
{
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        return GI->GetSubsystem<UMAConfigManager>();
    }
    return nullptr;
}

// ========== 初始化 ==========

void UMAEnvironmentManager::SpawnEnvironmentObjectsFromConfig()
{
    UE_LOG(LogMAEnvironmentManager, Log, TEXT("SpawnEnvironmentObjectsFromConfig called"));
    
    UMAConfigManager* ConfigMgr = GetConfigManager();
    if (!ConfigMgr)
    {
        UE_LOG(LogMAEnvironmentManager, Error, TEXT("ConfigManager not available"));
        return;
    }
    
    if (!ConfigMgr->IsConfigLoaded())
    {
        UE_LOG(LogMAEnvironmentManager, Error, TEXT("Config not loaded"));
        return;
    }

    UE_LOG(LogMAEnvironmentManager, Log, TEXT("EnvironmentObjects count from config: %d"), ConfigMgr->EnvironmentObjects.Num());

    // 生成所有环境对象 (charging_station, cargo, person, vehicle, boat, fire, smoke, wind)
    SpawnEnvironmentObjects();

    UE_LOG(LogMAEnvironmentManager, Log, TEXT("Spawned %d environment objects total"), SpawnedObjects.Num());
}

void UMAEnvironmentManager::SpawnEnvironmentObjects()
{
    UMAConfigManager* ConfigMgr = GetConfigManager();
    if (!ConfigMgr) return;

    for (const FMAEnvironmentObjectConfig& Config : ConfigMgr->EnvironmentObjects)
    {
        AActor* SpawnedObject = nullptr;

        if (Config.Type == TEXT("person"))
        {
            SpawnedObject = SpawnPerson(Config);
        }
        else if (Config.Type == TEXT("vehicle"))
        {
            SpawnedObject = SpawnVehicle(Config);
        }
        else if (Config.Type == TEXT("boat"))
        {
            SpawnedObject = SpawnBoat(Config);
        }
        else if (Config.Type == TEXT("cargo"))
        {
            SpawnedObject = SpawnCargo(Config);
        }
        else if (Config.Type == TEXT("charging_station"))
        {
            SpawnedObject = SpawnChargingStation(Config);
        }
        else if (Config.Type == TEXT("fire"))
        {
            SpawnedObject = SpawnFire(Config);
        }
        else if (Config.Type == TEXT("smoke"))
        {
            SpawnedObject = SpawnSmoke(Config);
        }
        else if (Config.Type == TEXT("wind"))
        {
            SpawnedObject = SpawnWind(Config);
        }
        else if (Config.Type == TEXT("assembly_component"))
        {
            SpawnedObject = SpawnComponent(Config);
        }
        else
        {
            UE_LOG(LogMAEnvironmentManager, Warning, TEXT("Unknown environment object type: %s (ID: %s)"), *Config.Type, *Config.ID);
            continue;
        }

        if (SpawnedObject)
        {
            FVector Loc = SpawnedObject->GetActorLocation();
            UE_LOG(LogMAEnvironmentManager, Log, TEXT("Spawned %s (%s) at (%.0f, %.0f, %.0f)"),
                *Config.Label, *Config.Type, Loc.X, Loc.Y, Loc.Z);
            
            SpawnedObjects.Add(SpawnedObject);
            
            // 绑定 GUID 到场景图节点
            if (UGameInstance* GI = GetWorld()->GetGameInstance())
            {
                if (UMASceneGraphManager* SceneGraphMgr = FMASceneGraphBootstrap::Resolve(GI))
                {
                    SceneGraphMgr->BindDynamicNodeGuid(Config.Label, SpawnedObject->GetActorGuid().ToString());
                }
            }
            
            OnEnvironmentObjectSpawned.Broadcast(SpawnedObject);
        }
        else
        {
            UE_LOG(LogMAEnvironmentManager, Error, TEXT("Failed to spawn %s (%s)"), *Config.Label, *Config.Type);
        }
    }
}

// ========== 环境对象查询 ==========

TArray<AActor*> UMAEnvironmentManager::GetEnvironmentObjectsByType(const FString& Type) const
{
    TArray<AActor*> Result;

    for (AActor* Object : SpawnedObjects)
    {
        if (IMAEnvironmentObject* EnvObj = Cast<IMAEnvironmentObject>(Object))
        {
            if (EnvObj->GetObjectType() == Type)
            {
                Result.Add(Object);
            }
        }
    }

    return Result;
}

AActor* UMAEnvironmentManager::GetEnvironmentObjectByLabel(const FString& Label) const
{
    for (AActor* Object : SpawnedObjects)
    {
        if (IMAEnvironmentObject* EnvObj = Cast<IMAEnvironmentObject>(Object))
        {
            if (EnvObj->GetObjectLabel() == Label)
            {
                return Object;
            }
        }
    }
    return nullptr;
}

// ========== 环境对象管理 ==========

bool UMAEnvironmentManager::DestroyEnvironmentObject(AActor* Object)
{
    if (!Object) return false;

    if (SpawnedObjects.Remove(Object) > 0)
    {
        // 从特定类型列表中移除
        SpawnedPersons.Remove(Cast<AMAPerson>(Object));
        SpawnedVehicles.Remove(Cast<AMAVehicle>(Object));
        SpawnedBoats.Remove(Cast<AMABoat>(Object));
        SpawnedPickupItems.Remove(Cast<AMACargo>(Object));
        SpawnedChargingStations.Remove(Cast<AMAChargingStation>(Object));
        SpawnedComponents.Remove(Cast<AMAComponent>(Object));
        SpawnedEffects.Remove(Object);

        OnEnvironmentObjectDestroyed.Broadcast(Object);
        Object->Destroy();
        return true;
    }
    return false;
}

// ========== 生成方法 ==========

AMAPerson* UMAEnvironmentManager::SpawnPerson(const FMAEnvironmentObjectConfig& Config)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 获取地面高度，加上 Capsule 半高让角色站在地面上
    // ACharacter 默认 Capsule 半高约 88-90cm
    FVector SpawnLocation = AdjustGroundSpawnHeight(Config.Position);
    SpawnLocation.Z += 90.f;

    AMAPerson* Person = GetWorld()->SpawnActor<AMAPerson>(
        AMAPerson::StaticClass(),
        SpawnLocation,
        Config.Rotation,
        SpawnParams
    );

    if (Person)
    {
        Person->Configure(Config);
        SpawnedPersons.Add(Person);
        
        // 检查是否需要生成附着火焰
        SpawnAttachedFireIfNeeded(Person, Config);
    }

    return Person;
}

AMAVehicle* UMAEnvironmentManager::SpawnVehicle(const FMAEnvironmentObjectConfig& Config)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 获取地面高度，加上 Capsule 半高 (50cm)
    FVector SpawnLocation = AdjustGroundSpawnHeight(Config.Position);
    // SpawnLocation.Z += 50.f;

    AMAVehicle* Vehicle = GetWorld()->SpawnActor<AMAVehicle>(
        AMAVehicle::StaticClass(),
        SpawnLocation,
        Config.Rotation,
        SpawnParams
    );

    if (Vehicle)
    {
        Vehicle->Configure(Config);
        SpawnedVehicles.Add(Vehicle);
        
        // 检查是否需要生成附着火焰
        SpawnAttachedFireIfNeeded(Vehicle, Config);
    }

    return Vehicle;
}

AMABoat* UMAEnvironmentManager::SpawnBoat(const FMAEnvironmentObjectConfig& Config)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 船只使用配置的位置，高度调整在 Configure 中根据 Mesh 尺寸计算
    FVector SpawnLocation = Config.Position;
    SpawnLocation.Z += 100.f;  // 临时偏移，Configure 中会重新设置正确高度

    AMABoat* Boat = GetWorld()->SpawnActor<AMABoat>(
        AMABoat::StaticClass(),
        SpawnLocation,
        Config.Rotation,
        SpawnParams
    );

    if (Boat)
    {
        Boat->Configure(Config);
        SpawnedBoats.Add(Boat);
        
        // 检查是否需要生成附着火焰
        SpawnAttachedFireIfNeeded(Boat, Config);
    }

    return Boat;
}

AMACargo* UMAEnvironmentManager::SpawnCargo(const FMAEnvironmentObjectConfig& Config)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    FVector SpawnLocation = AdjustGroundSpawnHeight(Config.Position);
    // 物理掉落，从地面上方 100cm 掉落
    SpawnLocation.Z += 100.f;

    AMACargo* Cargo = GetWorld()->SpawnActor<AMACargo>(
        AMACargo::StaticClass(),
        SpawnLocation,
        Config.Rotation,
        SpawnParams
    );

    if (Cargo)
    {
        Cargo->ItemName = Config.Label;
        Cargo->ObjectLabel = Config.Label;
        Cargo->ObjectType = Config.Type;
        Cargo->Features = Config.Features;

        // 从 Features 中读取颜色并设置
        if (const FString* ColorStr = Config.Features.Find(TEXT("color")))
        {
            FLinearColor ItemColor = ParseColorString(*ColorStr);
            Cargo->SetItemColor(ItemColor);
        }

        SpawnedPickupItems.Add(Cargo);
    }

    return Cargo;
}

AMAChargingStation* UMAEnvironmentManager::SpawnChargingStation(const FMAEnvironmentObjectConfig& Config)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    FVector SpawnLocation = AdjustGroundSpawnHeight(Config.Position);
    // 物理掉落，从地面上方 100cm 掉落
    SpawnLocation.Z += 100.f;

    AMAChargingStation* Station = GetWorld()->SpawnActor<AMAChargingStation>(
        AMAChargingStation::StaticClass(),
        SpawnLocation,
        Config.Rotation,
        SpawnParams
    );

    if (Station)
    {
        SpawnedChargingStations.Add(Station);
    }

    return Station;
}

AMAComponent* UMAEnvironmentManager::SpawnComponent(const FMAEnvironmentObjectConfig& Config)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 默认启用物理掉落，从地面上方 100cm 掉落
    FVector SpawnLocation = AdjustGroundSpawnHeight(Config.Position);
    SpawnLocation.Z += 100.f;

    AMAComponent* Component = GetWorld()->SpawnActor<AMAComponent>(
        AMAComponent::StaticClass(),
        SpawnLocation,
        Config.Rotation,
        SpawnParams
    );

    if (Component)
    {
        Component->Configure(Config);
        Component->EnablePhysics();
        SpawnedComponents.Add(Component);
    }

    return Component;
}

AMAFire* UMAEnvironmentManager::SpawnFire(const FMAEnvironmentObjectConfig& Config)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 火焰贴地生成
    FVector SpawnLocation = AdjustGroundSpawnHeight(Config.Position);
    SpawnLocation.Z += Config.Position.Z;

    AMAFire* Fire = GetWorld()->SpawnActor<AMAFire>(
        AMAFire::StaticClass(),
        SpawnLocation,
        Config.Rotation,
        SpawnParams
    );

    if (Fire)
    {
        Fire->Configure(Config);
        SpawnedEffects.Add(Fire);
    }

    return Fire;
}

AMASmoke* UMAEnvironmentManager::SpawnSmoke(const FMAEnvironmentObjectConfig& Config)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 烟雾在火焰上方生成
    FVector SpawnLocation = AdjustGroundSpawnHeight(Config.Position);
    SpawnLocation.Z += Config.Position.Z;

    AMASmoke* Smoke = GetWorld()->SpawnActor<AMASmoke>(
        AMASmoke::StaticClass(),
        SpawnLocation,
        Config.Rotation,
        SpawnParams
    );

    if (Smoke)
    {
        Smoke->Configure(Config);
        SpawnedEffects.Add(Smoke);
    }

    return Smoke;
}

AMAWind* UMAEnvironmentManager::SpawnWind(const FMAEnvironmentObjectConfig& Config)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 风特效贴地生成
    FVector SpawnLocation = AdjustGroundSpawnHeight(Config.Position);
    SpawnLocation.Z += Config.Position.Z;

    AMAWind* Wind = GetWorld()->SpawnActor<AMAWind>(
        AMAWind::StaticClass(),
        SpawnLocation,
        Config.Rotation,
        SpawnParams
    );

    if (Wind)
    {
        Wind->Configure(Config);
        SpawnedEffects.Add(Wind);
    }

    return Wind;
}

// ========== 辅助方法 ==========

FVector UMAEnvironmentManager::AdjustGroundSpawnHeight(FVector Location) const
{
    FHitResult HitResult;
    FVector TraceStart = FVector(Location.X, Location.Y, 10000.f);
    FVector TraceEnd = FVector(Location.X, Location.Y, -20000.f);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
    {
        Location.Z = HitResult.Location.Z + 5.f;
    }

    return Location;
}

FLinearColor UMAEnvironmentManager::ParseColorString(const FString& ColorString) const
{
    FString LowerColor = ColorString.ToLower();

    if (LowerColor == TEXT("red")) return FLinearColor::Red;
    if (LowerColor == TEXT("green")) return FLinearColor::Green;
    if (LowerColor == TEXT("blue")) return FLinearColor::Blue;
    if (LowerColor == TEXT("yellow")) return FLinearColor::Yellow;
    if (LowerColor == TEXT("white")) return FLinearColor::White;
    if (LowerColor == TEXT("black")) return FLinearColor::Black;
    if (LowerColor == TEXT("gray") || LowerColor == TEXT("grey")) return FLinearColor::Gray;
    if (LowerColor == TEXT("orange")) return FLinearColor(1.0f, 0.5f, 0.0f);
    if (LowerColor == TEXT("purple")) return FLinearColor(0.5f, 0.0f, 0.5f);
    if (LowerColor == TEXT("cyan")) return FLinearColor(0.0f, 1.0f, 1.0f);
    if (LowerColor == TEXT("magenta")) return FLinearColor(1.0f, 0.0f, 1.0f);
    if (LowerColor == TEXT("pink")) return FLinearColor(1.0f, 0.75f, 0.8f);
    if (LowerColor == TEXT("brown")) return FLinearColor(0.6f, 0.3f, 0.0f);
    if (LowerColor == TEXT("silver")) return FLinearColor(0.75f, 0.75f, 0.75f);

    // 尝试解析十六进制颜色
    FString HexColor = ColorString;
    if (HexColor.StartsWith(TEXT("#")))
    {
        HexColor = HexColor.RightChop(1);
    }

    if (HexColor.Len() == 6)
    {
        FColor ParsedColor = FColor::FromHex(HexColor);
        return FLinearColor(ParsedColor);
    }

    UE_LOG(LogMAEnvironmentManager, Warning, TEXT("Unknown color string: %s, using white"), *ColorString);
    return FLinearColor::White;
}

void UMAEnvironmentManager::SpawnAttachedFireIfNeeded(AActor* OwnerActor, const FMAEnvironmentObjectConfig& Config)
{
    // 检查 is_fire 特征
    const FString* IsFireValue = Config.Features.Find(TEXT("is_fire"));
    if (!IsFireValue || !IsFireValue->Equals(TEXT("true"), ESearchCase::IgnoreCase))
    {
        return;
    }
    
    IMAEnvironmentObject* EnvObject = Cast<IMAEnvironmentObject>(OwnerActor);
    if (!EnvObject)
    {
        return;
    }
    
    // 获取火焰缩放 (默认 3.0)
    float FireScale = 3.0f;
    const FString* FireScaleValue = Config.Features.Find(TEXT("fire_scale"));
    if (FireScaleValue)
    {
        FireScale = FCString::Atof(**FireScaleValue);
        if (FireScale <= 0.f) FireScale = 3.0f;
    }
    
    // 在对象上方生成火焰
    FVector FireLocation = OwnerActor->GetActorLocation();
    FireLocation.Z += 50.f;  // 火焰在对象上方 50cm
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    AMAFire* Fire = GetWorld()->SpawnActor<AMAFire>(
        AMAFire::StaticClass(),
        FireLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );
    
    if (Fire)
    {
        // 配置火焰
        FMAEnvironmentObjectConfig FireConfig;
        FireConfig.Label = FString::Printf(TEXT("%s_Fire"), *Config.Label);
        FireConfig.Type = TEXT("fire");
        FireConfig.Features.Add(TEXT("scale"), FString::Printf(TEXT("%.1f"), FireScale));
        Fire->Configure(FireConfig);
        
        // 附着到父对象
        Fire->AttachToActor(OwnerActor, FAttachmentTransformRules::KeepWorldTransform);
        
        // 设置环境对象的附着火焰引用
        EnvObject->SetAttachedFire(Fire);
        
        // 添加到特效列表
        SpawnedEffects.Add(Fire);
        
        UE_LOG(LogMAEnvironmentManager, Log, TEXT("Spawned attached fire for %s (scale=%.1f)"), 
            *Config.Label, FireScale);
    }
}
