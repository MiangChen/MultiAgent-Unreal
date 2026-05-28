// MAGameInstance.cpp
// 游戏实例实现

#include "Core/GameFlow/Bootstrap/MAGameInstance.h"
#include "Core/Config/MAConfigManager.h"
#include "Core/Comm/Runtime/MACommSubsystem.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAGameInstance, Log, All);

//=============================================================================
// 生命周期
//=============================================================================

void UMAGameInstance::Init()
{
    Super::Init();
    UE_LOG(LogMAGameInstance, Log, TEXT("MAGameInstance initialized"));
}

void UMAGameInstance::Shutdown()
{
    UE_LOG(LogMAGameInstance, Log, TEXT("MAGameInstance shutting down"));
    Super::Shutdown();
}

//=============================================================================
// Subsystem 访问
//=============================================================================

UMAConfigManager* UMAGameInstance::GetConfigManager() const
{
    return GetSubsystem<UMAConfigManager>();
}

UMACommSubsystem* UMAGameInstance::GetCommSubsystem() const
{
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

    return Cast<UMAGameInstance>(World->GetGameInstance());
}

//=============================================================================
// Setup 配置
//=============================================================================

void UMAGameInstance::SaveSetupConfig(const TMap<FString, int32>& AgentConfigs, const FString& SelectedScene)
{
    SetupAgentConfigs = AgentConfigs;
    SetupSelectedScene = SelectedScene;
    bSetupCompleted = true;

    UE_LOG(LogMAGameInstance, Log, TEXT("Setup config saved: Scene=%s"), *SetupSelectedScene);
    for (const auto& Pair : SetupAgentConfigs)
    {
        UE_LOG(LogMAGameInstance, Log, TEXT("  %s x%d"), *Pair.Key, Pair.Value);
    }
}

void UMAGameInstance::ClearSetupConfig()
{
    SetupAgentConfigs.Empty();
    SetupSelectedScene.Empty();
    bSetupCompleted = false;
    UE_LOG(LogMAGameInstance, Log, TEXT("Setup config cleared"));
}

int32 UMAGameInstance::GetSetupTotalAgentCount() const
{
    int32 Total = 0;
    for (const auto& Pair : SetupAgentConfigs)
    {
        Total += Pair.Value;
    }
    return Total;
}
