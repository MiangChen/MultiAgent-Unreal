// MAGameInstance.cpp
// 游戏实例实现
// Requirements: 8.3, 8.4

#include "MAGameInstance.h"
#include "MACommSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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

    UE_LOG(LogMAGameInstance, Log, TEXT("MAGameInstance initialized"));
    UE_LOG(LogMAGameInstance, Log, TEXT("  PlannerServerURL: %s"), *PlannerServerURL);
    UE_LOG(LogMAGameInstance, Log, TEXT("  bUseMockData: %s"), bUseMockData ? TEXT("true") : TEXT("false"));
    UE_LOG(LogMAGameInstance, Log, TEXT("  bDebugMode: %s"), bDebugMode ? TEXT("true") : TEXT("false"));
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
