// MATempDataManager.cpp
// 临时数据管理器实现

#include "MATempDataManager.h"
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
const FString UMATempDataManager::SkillListFileName = TEXT("skill_list_temp.json");
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
    CachedSkillListFilePath = GetSkillListFilePath();

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

bool UMATempDataManager::SaveTaskGraph(const FMATaskGraphData& Data)
{
    // 确保目录存在
    if (!EnsureDataDirectoryExists())
    {
        UE_LOG(LogMATempData, Error, TEXT("SaveTaskGraph: Failed to ensure data directory exists"));
        return false;
    }

    // 序列化为 JSON
    FString JsonContent = Data.ToJson();

    // 写入文件
    if (!WriteJsonToFile(CachedTaskGraphFilePath, JsonContent))
    {
        UE_LOG(LogMATempData, Error, TEXT("SaveTaskGraph: Failed to write file: %s"), *CachedTaskGraphFilePath);
        return false;
    }

    UE_LOG(LogMATempData, Log, TEXT("SaveTaskGraph: Successfully saved to %s"), *CachedTaskGraphFilePath);

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

    // 解析 JSON
    FString ErrorMessage;
    if (!FMATaskGraphData::FromJsonWithError(JsonContent, OutData, ErrorMessage))
    {
        // 尝试使用 response 格式解析
        if (!FMATaskGraphData::FromResponseJson(JsonContent, OutData, ErrorMessage))
        {
            UE_LOG(LogMATempData, Error, TEXT("LoadTaskGraph: Failed to parse JSON: %s"), *ErrorMessage);
            OutData = FMATaskGraphData();
            return false;
        }
    }

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
// 技能列表操作
//=============================================================================

bool UMATempDataManager::SaveSkillList(const FMASkillAllocationData& Data)
{
    // 确保目录存在
    if (!EnsureDataDirectoryExists())
    {
        UE_LOG(LogMATempData, Error, TEXT("SaveSkillList: Failed to ensure data directory exists"));
        return false;
    }

    // 序列化为 JSON
    FString JsonContent = Data.ToJson();

    // 写入文件
    if (!WriteJsonToFile(CachedSkillListFilePath, JsonContent))
    {
        UE_LOG(LogMATempData, Error, TEXT("SaveSkillList: Failed to write file: %s"), *CachedSkillListFilePath);
        return false;
    }

    UE_LOG(LogMATempData, Log, TEXT("SaveSkillList: Successfully saved to %s"), *CachedSkillListFilePath);

    // Update cache
    CachedSkillListData = Data;
    bSkillListCacheValid = true;

    // 广播数据变更事件
    OnSkillListChanged.Broadcast(Data);

    return true;
}

bool UMATempDataManager::LoadSkillList(FMASkillAllocationData& OutData)
{
    // 如果缓存有效，直接返回缓存数据
    if (bSkillListCacheValid)
    {
        OutData = CachedSkillListData;
        UE_LOG(LogMATempData, Log, TEXT("LoadSkillList: Returning cached data (Name: %s, TimeSteps: %d)"),
            *OutData.Name, OutData.Data.Num());
        return true;
    }
    
    // 检查文件是否存在
    if (!SkillListFileExists())
    {
        UE_LOG(LogMATempData, Warning, TEXT("LoadSkillList: File does not exist: %s"), *CachedSkillListFilePath);
        OutData = FMASkillAllocationData();
        return false;
    }

    // 读取文件内容
    FString JsonContent;
    if (!ReadJsonFromFile(CachedSkillListFilePath, JsonContent))
    {
        UE_LOG(LogMATempData, Error, TEXT("LoadSkillList: Failed to read file: %s"), *CachedSkillListFilePath);
        OutData = FMASkillAllocationData();
        return false;
    }

    // 解析 JSON
    FString ErrorMessage;
    if (!FMASkillAllocationData::FromJson(JsonContent, OutData, ErrorMessage))
    {
        UE_LOG(LogMATempData, Error, TEXT("LoadSkillList: Failed to parse JSON: %s"), *ErrorMessage);
        OutData = FMASkillAllocationData();
        return false;
    }

    // 更新缓存
    CachedSkillListData = OutData;
    bSkillListCacheValid = true;

    UE_LOG(LogMATempData, Log, TEXT("LoadSkillList: Successfully loaded from %s (Name: %s, TimeSteps: %d)"),
        *CachedSkillListFilePath, *OutData.Name, OutData.Data.Num());

    return true;
}

FString UMATempDataManager::GetSkillListJson() const
{
    FString JsonContent;
    if (ReadJsonFromFile(CachedSkillListFilePath, JsonContent))
    {
        return JsonContent;
    }
    return FString();
}

bool UMATempDataManager::SkillListFileExists() const
{
    return FPlatformFileManager::Get().GetPlatformFile().FileExists(*CachedSkillListFilePath);
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

FString UMATempDataManager::GetSkillListFilePath() const
{
    if (!CachedSkillListFilePath.IsEmpty())
    {
        return CachedSkillListFilePath;
    }
    return FPaths::Combine(GetDataDirectoryPath(), SkillListFileName);
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
    bSkillListFileChanged = false;

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
        else if (FileName == SkillListFileName)
        {
            bSkillListFileChanged = true;
            UE_LOG(LogMATempData, Verbose, TEXT("OnFileChanged: Skill list file changed"));
        }
    }

    // 使用防抖机制延迟处理
    if (bTaskGraphFileChanged || bSkillListFileChanged)
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

    // 处理技能列表文件变化
    if (bSkillListFileChanged)
    {
        bSkillListFileChanged = false;
        
        // Invalidate cache when file changes
        bSkillListCacheValid = false;

        FMASkillAllocationData Data;
        if (LoadSkillList(Data))
        {
            UE_LOG(LogMATempData, Log, TEXT("ProcessFileChange: Reloaded skill list, broadcasting event"));
            OnSkillListChanged.Broadcast(Data);
        }
    }
}

//=============================================================================
// 技能状态实时更新
//=============================================================================

void UMATempDataManager::BroadcastSkillStatusUpdate(int32 TimeStep, const FString& RobotId, ESkillExecutionStatus NewStatus)
{
    UE_LOG(LogMATempData, Verbose, TEXT("BroadcastSkillStatusUpdate: TimeStep=%d, RobotId=%s, Status=%d"),
        TimeStep, *RobotId, static_cast<int32>(NewStatus));
    
    // Update cached data if valid
    if (bSkillListCacheValid)
    {
        FMASkillAssignment* Skill = CachedSkillListData.FindSkill(TimeStep, RobotId);
        if (Skill)
        {
            Skill->Status = NewStatus;
            UE_LOG(LogMATempData, Verbose, TEXT("BroadcastSkillStatusUpdate: Updated cached skill status"));
        }
    }
    
    OnSkillStatusUpdated.Broadcast(TimeStep, RobotId, NewStatus);
}
