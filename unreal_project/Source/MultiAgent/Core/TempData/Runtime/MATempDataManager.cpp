// MATempDataManager.cpp
// 临时数据管理器实现

#include "MATempDataManager.h"
#include "Core/SkillAllocation/Application/MASkillAllocationUseCases.h"
#include "Core/TaskGraph/Application/MATaskGraphUseCases.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "Modules/ModuleManager.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogMATempData);

//=============================================================================
// 静态常量定义
//=============================================================================

const FString UMATempDataManager::TaskGraphFileName = TEXT("task_graph_temp.json");
const FString UMATempDataManager::SkillAllocationFileName = TEXT("skill_allocation_temp.json");
const FString UMATempDataManager::DataDirectoryName = TEXT("data");

//=============================================================================
// 生命周期
//=============================================================================

void UMATempDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 缓存路径
    CachedDataDirectory = GetDataDirectoryPath();
    CachedTaskGraphFilePath = GetTaskGraphFilePath();
    CachedSkillAllocationFilePath = GetSkillAllocationFilePath();

    // 确保数据目录存在
    if (EnsureDataDirectoryExists())
    {
        UE_LOG(LogMATempData, Log, TEXT("TempDataManager initialized. Data directory: %s"), *CachedDataDirectory);
    }
    else
    {
        UE_LOG(LogMATempData, Error, TEXT("Failed to create data directory: %s"), *CachedDataDirectory);
    }
}

void UMATempDataManager::Deinitialize()
{
    // 停止文件监听
    StopFileWatching();

    UE_LOG(LogMATempData, Log, TEXT("TempDataManager deinitialized"));

    Super::Deinitialize();
}

//=============================================================================
// 任务图操作
//=============================================================================

bool UMATempDataManager::SaveTaskGraph(const FMATaskGraphData& Data, const FString& ReviewMessageId)
{
    // 确保目录存在
    if (!EnsureDataDirectoryExists())
    {
        UE_LOG(LogMATempData, Error, TEXT("SaveTaskGraph: Failed to ensure data directory exists"));
        return false;
    }

    // 序列化为 JSON
    const FString JsonContent = FTaskGraphUseCases::Serialize(Data);

    // 写入文件
    if (!WriteJsonToFile(CachedTaskGraphFilePath, JsonContent))
    {
        UE_LOG(LogMATempData, Error, TEXT("SaveTaskGraph: Failed to write file: %s"), *CachedTaskGraphFilePath);
        return false;
    }

    UE_LOG(LogMATempData, Log, TEXT("SaveTaskGraph: Successfully saved to %s"), *CachedTaskGraphFilePath);

    CachedTaskGraphReviewMessageId = ReviewMessageId;

    // 广播数据变更事件
    OnTaskGraphChanged.Broadcast(Data);

    return true;
}

bool UMATempDataManager::LoadTaskGraph(FMATaskGraphData& OutData)
{
    // 检查文件是否存在
    if (!TaskGraphFileExists())
    {
        UE_LOG(LogMATempData, Warning, TEXT("LoadTaskGraph: File does not exist: %s"), *CachedTaskGraphFilePath);
        OutData = FMATaskGraphData();
        return false;
    }

    // 读取文件内容
    FString JsonContent;
    if (!ReadJsonFromFile(CachedTaskGraphFilePath, JsonContent))
    {
        UE_LOG(LogMATempData, Error, TEXT("LoadTaskGraph: Failed to read file: %s"), *CachedTaskGraphFilePath);
        OutData = FMATaskGraphData();
        return false;
    }

    const FTaskGraphLoadResult LoadResult = FTaskGraphUseCases::ParseFlexibleJson(JsonContent);
    if (!LoadResult.bSuccess)
    {
        UE_LOG(LogMATempData, Error, TEXT("LoadTaskGraph: Failed to parse JSON: %s"), *LoadResult.Feedback.Message);
        OutData = FMATaskGraphData();
        return false;
    }
    OutData = LoadResult.Data;

    UE_LOG(LogMATempData, Log, TEXT("LoadTaskGraph: Successfully loaded from %s (Nodes: %d, Edges: %d)"),
        *CachedTaskGraphFilePath, OutData.Nodes.Num(), OutData.Edges.Num());

    return true;
}

FString UMATempDataManager::GetTaskGraphJson() const
{
    FString JsonContent;
    if (ReadJsonFromFile(CachedTaskGraphFilePath, JsonContent))
    {
        return JsonContent;
    }
    return FString();
}

bool UMATempDataManager::TaskGraphFileExists() const
{
    return FPlatformFileManager::Get().GetPlatformFile().FileExists(*CachedTaskGraphFilePath);
}

//=============================================================================
// 技能分配操作
//=============================================================================

bool UMATempDataManager::SaveSkillAllocation(const FMASkillAllocationData& Data, const FString& ReviewMessageId)
{
    // 确保目录存在
    if (!EnsureDataDirectoryExists())
    {
        UE_LOG(LogMATempData, Error, TEXT("SaveSkillAllocation: Failed to ensure data directory exists"));
        return false;
    }

    // 序列化为 JSON
    const FString JsonContent = FMASkillAllocationUseCases::SerializeJson(Data);

    // 写入文件
    if (!WriteJsonToFile(CachedSkillAllocationFilePath, JsonContent))
    {
        UE_LOG(LogMATempData, Error, TEXT("SaveSkillAllocation: Failed to write file: %s"), *CachedSkillAllocationFilePath);
        return false;
    }

    UE_LOG(LogMATempData, Log, TEXT("SaveSkillAllocation: Successfully saved to %s"), *CachedSkillAllocationFilePath);

    // Update cache
    CachedSkillAllocationData = Data;
    bSkillAllocationCacheValid = true;
    CachedSkillAllocationReviewMessageId = ReviewMessageId;

    // 广播数据变更事件
    OnSkillAllocationChanged.Broadcast(Data);

    return true;
}

bool UMATempDataManager::LoadSkillAllocation(FMASkillAllocationData& OutData)
{
    // 如果缓存有效，直接返回缓存数据
    if (bSkillAllocationCacheValid)
    {
        OutData = CachedSkillAllocationData;
        UE_LOG(LogMATempData, Log, TEXT("LoadSkillAllocation: Returning cached data (Name: %s, TimeSteps: %d)"),
            *OutData.Name, OutData.Data.Num());
        return true;
    }
    
    // 检查文件是否存在
    if (!SkillAllocationFileExists())
    {
        UE_LOG(LogMATempData, Warning, TEXT("LoadSkillAllocation: File does not exist: %s"), *CachedSkillAllocationFilePath);
        OutData = FMASkillAllocationData();
        return false;
    }

    // 读取文件内容
    FString JsonContent;
    if (!ReadJsonFromFile(CachedSkillAllocationFilePath, JsonContent))
    {
        UE_LOG(LogMATempData, Error, TEXT("LoadSkillAllocation: Failed to read file: %s"), *CachedSkillAllocationFilePath);
        OutData = FMASkillAllocationData();
        return false;
    }

    // 解析 JSON
    FString ErrorMessage;
    if (!FMASkillAllocationUseCases::ParseJson(JsonContent, OutData, ErrorMessage))
    {
        UE_LOG(LogMATempData, Error, TEXT("LoadSkillAllocation: Failed to parse JSON: %s"), *ErrorMessage);
        OutData = FMASkillAllocationData();
        return false;
    }

    // 更新缓存
    CachedSkillAllocationData = OutData;
    bSkillAllocationCacheValid = true;

    UE_LOG(LogMATempData, Log, TEXT("LoadSkillAllocation: Successfully loaded from %s (Name: %s, TimeSteps: %d)"),
        *CachedSkillAllocationFilePath, *OutData.Name, OutData.Data.Num());

    return true;
}

FString UMATempDataManager::GetSkillAllocationJson() const
{
    FString JsonContent;
    if (ReadJsonFromFile(CachedSkillAllocationFilePath, JsonContent))
    {
        return JsonContent;
    }
    return FString();
}

bool UMATempDataManager::SkillAllocationFileExists() const
{
    return FPlatformFileManager::Get().GetPlatformFile().FileExists(*CachedSkillAllocationFilePath);
}

//=============================================================================
// 文件路径
//=============================================================================

FString UMATempDataManager::GetTaskGraphFilePath() const
{
    if (!CachedTaskGraphFilePath.IsEmpty())
    {
        return CachedTaskGraphFilePath;
    }
    return FPaths::Combine(GetDataDirectoryPath(), TaskGraphFileName);
}

FString UMATempDataManager::GetSkillAllocationFilePath() const
{
    if (!CachedSkillAllocationFilePath.IsEmpty())
    {
        return CachedSkillAllocationFilePath;
    }
    return FPaths::Combine(GetDataDirectoryPath(), SkillAllocationFileName);
}

FString UMATempDataManager::GetDataDirectoryPath() const
{
    if (!CachedDataDirectory.IsEmpty())
    {
        return CachedDataDirectory;
    }
    // 使用项目根目录下的 data 文件夹
    return FPaths::Combine(FPaths::ProjectDir(), DataDirectoryName);
}


//=============================================================================
// 文件监听控制
//=============================================================================

void UMATempDataManager::StartFileWatching()
{
    if (bIsFileWatching)
    {
        UE_LOG(LogMATempData, Warning, TEXT("StartFileWatching: Already watching"));
        return;
    }

    FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
    IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

    if (!DirectoryWatcher)
    {
        UE_LOG(LogMATempData, Error, TEXT("StartFileWatching: Failed to get DirectoryWatcher"));
        return;
    }

    // 注册目录监听
    FDelegateHandle Handle;
    IDirectoryWatcher::FDirectoryChanged Callback = IDirectoryWatcher::FDirectoryChanged::CreateUObject(
        this, &UMATempDataManager::OnFileChanged);

    if (DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(
        CachedDataDirectory,
        Callback,
        Handle,
        IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges))
    {
        FileWatchHandle = Handle;
        bIsFileWatching = true;
        UE_LOG(LogMATempData, Log, TEXT("StartFileWatching: Started watching directory: %s"), *CachedDataDirectory);
    }
    else
    {
        UE_LOG(LogMATempData, Error, TEXT("StartFileWatching: Failed to register directory watcher for: %s"), *CachedDataDirectory);
    }
}

void UMATempDataManager::StopFileWatching()
{
    if (!bIsFileWatching)
    {
        return;
    }

    FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
    IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

    if (DirectoryWatcher && FileWatchHandle.IsValid())
    {
        DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(CachedDataDirectory, FileWatchHandle);
        FileWatchHandle.Reset();
    }

    // 清除防抖定时器
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UWorld* World = GameInstance->GetWorld())
        {
            World->GetTimerManager().ClearTimer(DebounceTimerHandle);
        }
    }

    bIsFileWatching = false;
    bTaskGraphFileChanged = false;
    bSkillAllocationFileChanged = false;

    UE_LOG(LogMATempData, Log, TEXT("StopFileWatching: Stopped watching directory"));
}

//=============================================================================
// 内部方法
//=============================================================================

bool UMATempDataManager::EnsureDataDirectoryExists()
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    if (PlatformFile.DirectoryExists(*CachedDataDirectory))
    {
        return true;
    }

    // 创建目录
    if (PlatformFile.CreateDirectoryTree(*CachedDataDirectory))
    {
        UE_LOG(LogMATempData, Log, TEXT("Created data directory: %s"), *CachedDataDirectory);
        return true;
    }

    UE_LOG(LogMATempData, Error, TEXT("Failed to create data directory: %s"), *CachedDataDirectory);
    return false;
}

bool UMATempDataManager::WriteJsonToFile(const FString& FilePath, const FString& JsonContent)
{
    // 使用格式化的 JSON 输出以便于调试
    if (FFileHelper::SaveStringToFile(JsonContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        return true;
    }

    UE_LOG(LogMATempData, Error, TEXT("WriteJsonToFile: Failed to write to %s"), *FilePath);
    return false;
}

bool UMATempDataManager::ReadJsonFromFile(const FString& FilePath, FString& OutJsonContent) const
{
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        return false;
    }

    if (FFileHelper::LoadFileToString(OutJsonContent, *FilePath))
    {
        return true;
    }

    UE_LOG(LogMATempData, Error, TEXT("ReadJsonFromFile: Failed to read from %s"), *FilePath);
    return false;
}

void UMATempDataManager::OnFileChanged(const TArray<FFileChangeData>& ChangedFiles)
{
    for (const FFileChangeData& FileChange : ChangedFiles)
    {
        FString FileName = FPaths::GetCleanFilename(FileChange.Filename);

        if (FileName == TaskGraphFileName)
        {
            bTaskGraphFileChanged = true;
            UE_LOG(LogMATempData, Verbose, TEXT("OnFileChanged: Task graph file changed"));
        }
        else if (FileName == SkillAllocationFileName)
        {
            bSkillAllocationFileChanged = true;
            UE_LOG(LogMATempData, Verbose, TEXT("OnFileChanged: Skill allocation file changed"));
        }
    }

    // 使用防抖机制延迟处理
    if (bTaskGraphFileChanged || bSkillAllocationFileChanged)
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UWorld* World = GameInstance->GetWorld())
            {
                World->GetTimerManager().ClearTimer(DebounceTimerHandle);
                World->GetTimerManager().SetTimer(
                    DebounceTimerHandle,
                    this,
                    &UMATempDataManager::ProcessFileChange,
                    DebounceDelay,
                    false);
            }
        }
    }
}

void UMATempDataManager::ProcessFileChange()
{
    // 处理任务图文件变化
    if (bTaskGraphFileChanged)
    {
        bTaskGraphFileChanged = false;

        FMATaskGraphData Data;
        if (LoadTaskGraph(Data))
        {
            UE_LOG(LogMATempData, Log, TEXT("ProcessFileChange: Reloaded task graph, broadcasting event"));
            OnTaskGraphChanged.Broadcast(Data);
        }
    }

    // 处理技能分配文件变化
    if (bSkillAllocationFileChanged)
    {
        bSkillAllocationFileChanged = false;
        
        // Invalidate cache when file changes
        bSkillAllocationCacheValid = false;

        FMASkillAllocationData Data;
        if (LoadSkillAllocation(Data))
        {
            UE_LOG(LogMATempData, Log, TEXT("ProcessFileChange: Reloaded skill allocation, broadcasting event"));
            OnSkillAllocationChanged.Broadcast(Data);
        }
    }
}

//=============================================================================
// 技能状态实时更新
//=============================================================================

void UMATempDataManager::BroadcastSkillStatusUpdate(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    UE_LOG(LogMATempData, Log, TEXT("BroadcastSkillStatusUpdate: TimeStep=%d, RobotId=%s, Status=%d"),
        TimeStep, *RobotId, static_cast<int32>(NewStatus));
    
    // Update cached data if valid
    if (bSkillAllocationCacheValid)
    {
        FMASkillAssignment* Skill = CachedSkillAllocationData.FindSkill(TimeStep, RobotId);
        if (Skill)
        {
            Skill->Status = NewStatus;
            UE_LOG(LogMATempData, Log, TEXT("BroadcastSkillStatusUpdate: Updated cached skill status"));
        }
        else
        {
            UE_LOG(LogMATempData, Warning, TEXT("BroadcastSkillStatusUpdate: Skill not found in cache for TimeStep=%d, RobotId=%s"),
                TimeStep, *RobotId);
        }
    }
    else
    {
        UE_LOG(LogMATempData, Warning, TEXT("BroadcastSkillStatusUpdate: Cache is not valid"));
    }
    
    UE_LOG(LogMATempData, Log, TEXT("BroadcastSkillStatusUpdate: Broadcasting OnSkillStatusUpdated event"));
    OnSkillStatusUpdated.Broadcast(TimeStep, RobotId, NewStatus);
}
