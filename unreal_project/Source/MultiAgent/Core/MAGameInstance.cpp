// MAGameInstance.cpp
// 游戏实例实现
// Requirements: 8.3, 8.4

#include "MAGameInstance.h"
#include "MACommSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAGameInstance, Log, All);

//=============================================================================
// 构造函数
//=============================================================================

UMAGameInstance::UMAGameInstance()
{
    // 默认配置已在头文件中设置
}

//=============================================================================
// 生命周期
//=============================================================================

void UMAGameInstance::Init()
{
    Super::Init();

    // 从 JSON 加载配置
    LoadConfigFromJSON();

    UE_LOG(LogMAGameInstance, Log, TEXT("MAGameInstance initialized"));
    UE_LOG(LogMAGameInstance, Log, TEXT("  PlannerServerURL: %s"), *PlannerServerURL);
    UE_LOG(LogMAGameInstance, Log, TEXT("  bUseMockData: %s"), bUseMockData ? TEXT("true") : TEXT("false"));
    UE_LOG(LogMAGameInstance, Log, TEXT("  bDebugMode: %s"), bDebugMode ? TEXT("true") : TEXT("false"));
    UE_LOG(LogMAGameInstance, Log, TEXT("  ConfiguredMapPath: %s"), *ConfiguredMapPath);
}

void UMAGameInstance::Shutdown()
{
    UE_LOG(LogMAGameInstance, Log, TEXT("MAGameInstance shutting down"));

    Super::Shutdown();
}

//=============================================================================
// Subsystem 访问
//=============================================================================

UMACommSubsystem* UMAGameInstance::GetCommSubsystem() const
{
    // CommSubsystem 是 GameInstanceSubsystem，通过 GameInstance 获取
    return GetSubsystem<UMACommSubsystem>();
}

//=============================================================================
// 静态访问方法
//=============================================================================

UMAGameInstance* UMAGameInstance::GetMAGameInstance(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    return Cast<UMAGameInstance>(GameInstance);
}

//=============================================================================
// JSON 配置加载
//=============================================================================

void UMAGameInstance::LoadConfigFromJSON()
{
    // 配置文件路径: 项目根目录/config/SimConfig.json (与 agents.json 并列)
    FString ConfigPath = FPaths::ProjectDir() / TEXT("config/SimConfig.json");
    
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
    {
        UE_LOG(LogMAGameInstance, Warning, TEXT("Failed to load config file: %s"), *ConfigPath);
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogMAGameInstance, Error, TEXT("Failed to parse JSON config: %s"), *ConfigPath);
        return;
    }

    // 读取配置项
    if (JsonObject->HasField(TEXT("DefaultMap")))
    {
        ConfiguredMapPath = JsonObject->GetStringField(TEXT("DefaultMap"));
    }

    if (JsonObject->HasField(TEXT("PlannerServerURL")))
    {
        PlannerServerURL = JsonObject->GetStringField(TEXT("PlannerServerURL"));
    }

    if (JsonObject->HasField(TEXT("bUseMockData")))
    {
        bUseMockData = JsonObject->GetBoolField(TEXT("bUseMockData"));
    }

    if (JsonObject->HasField(TEXT("bDebugMode")))
    {
        bDebugMode = JsonObject->GetBoolField(TEXT("bDebugMode"));
    }

    // 读取 SpectatorStart 配置
    if (const TSharedPtr<FJsonObject>* SpectatorObj; JsonObject->TryGetObjectField(TEXT("SpectatorStart"), SpectatorObj))
    {
        if (const TArray<TSharedPtr<FJsonValue>>* PosArray; (*SpectatorObj)->TryGetArrayField(TEXT("Position"), PosArray))
        {
            if (PosArray->Num() >= 3)
            {
                SpectatorStartPosition = FVector(
                    (*PosArray)[0]->AsNumber(),
                    (*PosArray)[1]->AsNumber(),
                    (*PosArray)[2]->AsNumber()
                );
            }
        }
        
        if (const TArray<TSharedPtr<FJsonValue>>* RotArray; (*SpectatorObj)->TryGetArrayField(TEXT("Rotation"), RotArray))
        {
            if (RotArray->Num() >= 3)
            {
                SpectatorStartRotation = FRotator(
                    (*RotArray)[0]->AsNumber(),
                    (*RotArray)[1]->AsNumber(),
                    (*RotArray)[2]->AsNumber()
                );
            }
        }
        
        UE_LOG(LogMAGameInstance, Log, TEXT("  SpectatorStart: (%.0f, %.0f, %.0f) Rot: (%.0f, %.0f, %.0f)"),
            SpectatorStartPosition.X, SpectatorStartPosition.Y, SpectatorStartPosition.Z,
            SpectatorStartRotation.Pitch, SpectatorStartRotation.Yaw, SpectatorStartRotation.Roll);
    }

    UE_LOG(LogMAGameInstance, Log, TEXT("Loaded config from: %s"), *ConfigPath);
}

//=============================================================================
// 地图加载
//=============================================================================

void UMAGameInstance::LoadConfiguredMap()
{
    if (ConfiguredMapPath.IsEmpty())
    {
        UE_LOG(LogMAGameInstance, Warning, TEXT("No configured map path"));
        return;
    }

    if (bHasLoadedConfiguredMap)
    {
        UE_LOG(LogMAGameInstance, Log, TEXT("Configured map already loaded"));
        return;
    }

    // 检查当前地图是否已经是目标地图
    UWorld* World = GetWorld();
    if (World)
    {
        FString CurrentMapName = World->GetMapName();
        // 移除 UEDPIE_ 前缀 (PIE 模式下的前缀)
        CurrentMapName.RemoveFromStart(TEXT("UEDPIE_"));
        
        // 提取目标地图名称
        FString TargetMapName = FPaths::GetBaseFilename(ConfiguredMapPath);
        
        if (CurrentMapName.Contains(TargetMapName))
        {
            UE_LOG(LogMAGameInstance, Log, TEXT("Already on configured map: %s"), *TargetMapName);
            bHasLoadedConfiguredMap = true;
            return;
        }
    }

    UE_LOG(LogMAGameInstance, Log, TEXT("Loading configured map: %s"), *ConfiguredMapPath);
    
    bHasLoadedConfiguredMap = true;
    UGameplayStatics::OpenLevel(this, FName(*ConfiguredMapPath));
}
