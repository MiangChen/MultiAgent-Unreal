// MAEditModeManager.cpp
// Edit Mode 管理器实现
// Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 3.1, 3.2, 3.4, 3.5, 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7

#include "MAEditModeManager.h"
#include "../Environment/MAPointOfInterest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogMAEditMode);

//=============================================================================
// 生命周期
//=============================================================================

void UMAEditModeManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogMAEditMode, Log, TEXT("MAEditModeManager initializing"));
    
    // 检查源场景图文件是否存在
    FString SourcePath = GetSourceSceneGraphPath();
    if (FPaths::FileExists(SourcePath))
    {
        bEditModeAvailable = true;
        UE_LOG(LogMAEditMode, Log, TEXT("Source scene graph found: %s"), *SourcePath);
    }
    else
    {
        bEditModeAvailable = false;
        UE_LOG(LogMAEditMode, Error, TEXT("Source scene graph file not found: %s"), *SourcePath);
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("MAEditModeManager initialized, EditModeAvailable=%s"), 
        bEditModeAvailable ? TEXT("true") : TEXT("false"));
}

void UMAEditModeManager::Deinitialize()
{
    UE_LOG(LogMAEditMode, Log, TEXT("MAEditModeManager deinitializing"));
    
    // 清理选择状态
    ClearSelection();
    
    // 销毁所有 POI
    DestroyAllPOIs();
    
    // 删除临时场景图文件
    DeleteTempSceneGraph();
    
    // 清理数据
    TempSceneGraphData.Reset();
    
    Super::Deinitialize();
}

//=============================================================================
// 临时场景图管理
// Requirements: 1.1, 1.2, 1.3, 1.4, 1.5
//=============================================================================

FString UMAEditModeManager::GetSourceSceneGraphPath() const
{
    // 源场景图文件路径
    return FPaths::ProjectDir() / TEXT("datasets/scene_graph_cyberworld.json");
}

bool UMAEditModeManager::CreateTempSceneGraph()
{
    // Requirements: 1.1, 1.5
    
    if (!bEditModeAvailable)
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreateTempSceneGraph: Edit Mode not available"));
        return false;
    }
    
    FString SourcePath = GetSourceSceneGraphPath();
    
    // 检查源文件是否存在
    if (!FPaths::FileExists(SourcePath))
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreateTempSceneGraph: Source file not found: %s"), *SourcePath);
        bEditModeAvailable = false;
        return false;
    }
    
    // Requirements: 1.5 - 生成临时文件路径，包含时间戳
    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
    FString TempDir = FPaths::ProjectSavedDir() / TEXT("Temp");
    TempSceneGraphPath = TempDir / FString::Printf(TEXT("temp_scene_graph_%s.json"), *Timestamp);
    
    // 确保目录存在
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*TempDir))
    {
        PlatformFile.CreateDirectoryTree(*TempDir);
    }
    
    // 复制源文件到临时文件
    if (!PlatformFile.CopyFile(*TempSceneGraphPath, *SourcePath))
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreateTempSceneGraph: Failed to copy file from %s to %s"), 
            *SourcePath, *TempSceneGraphPath);
        return false;
    }
    
    // Requirements: 1.2 - 加载临时场景图数据
    if (!LoadTempSceneGraph())
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreateTempSceneGraph: Failed to load temp scene graph"));
        DeleteTempSceneGraph();
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("CreateTempSceneGraph: Created temp scene graph at %s"), *TempSceneGraphPath);
    
    // 广播变化事件
    OnTempSceneGraphChanged.Broadcast();
    
    return true;
}

void UMAEditModeManager::DeleteTempSceneGraph()
{
    // Requirements: 1.4
    
    if (TempSceneGraphPath.IsEmpty())
    {
        return;
    }
    
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    if (PlatformFile.FileExists(*TempSceneGraphPath))
    {
        if (PlatformFile.DeleteFile(*TempSceneGraphPath))
        {
            UE_LOG(LogMAEditMode, Log, TEXT("DeleteTempSceneGraph: Deleted %s"), *TempSceneGraphPath);
        }
        else
        {
            UE_LOG(LogMAEditMode, Warning, TEXT("DeleteTempSceneGraph: Failed to delete %s"), *TempSceneGraphPath);
        }
    }
    
    TempSceneGraphPath.Empty();
    TempSceneGraphData.Reset();
}

FString UMAEditModeManager::GetTempSceneGraphPath() const
{
    // Requirements: 1.2
    return TempSceneGraphPath;
}

FString UMAEditModeManager::GetTempSceneGraphJson() const
{
    if (!TempSceneGraphData.IsValid())
    {
        return TEXT("");
    }
    
    FString JsonString;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = 
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonString);
    
    if (FJsonSerializer::Serialize(TempSceneGraphData.ToSharedRef(), Writer))
    {
        return JsonString;
    }
    
    return TEXT("");
}

bool UMAEditModeManager::LoadTempSceneGraph()
{
    if (TempSceneGraphPath.IsEmpty())
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("LoadTempSceneGraph: Path is empty"));
        return false;
    }
    
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *TempSceneGraphPath))
    {
        UE_LOG(LogMAEditMode, Error, TEXT("LoadTempSceneGraph: Failed to read file: %s"), *TempSceneGraphPath);
        return false;
    }
    
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, TempSceneGraphData) || !TempSceneGraphData.IsValid())
    {
        UE_LOG(LogMAEditMode, Error, TEXT("LoadTempSceneGraph: Failed to parse JSON"));
        return false;
    }
    
    // 计算下一个可用的 Node ID
    NextNodeId = 1;
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (TempSceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
        {
            if (NodeValue.IsValid() && NodeValue->Type == EJson::Object)
            {
                TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
                FString NodeId;
                if (NodeObject->TryGetStringField(TEXT("id"), NodeId))
                {
                    // 尝试解析数字 ID
                    int32 IdNum = FCString::Atoi(*NodeId);
                    if (IdNum >= NextNodeId)
                    {
                        NextNodeId = IdNum + 1;
                    }
                }
            }
        }
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("LoadTempSceneGraph: Loaded, NextNodeId=%d"), NextNodeId);
    return true;
}

bool UMAEditModeManager::SaveTempSceneGraph()
{
    if (TempSceneGraphPath.IsEmpty() || !TempSceneGraphData.IsValid())
    {
        UE_LOG(LogMAEditMode, Error, TEXT("SaveTempSceneGraph: Invalid state"));
        return false;
    }
    
    FString JsonString;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = 
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonString);
    
    if (!FJsonSerializer::Serialize(TempSceneGraphData.ToSharedRef(), Writer))
    {
        UE_LOG(LogMAEditMode, Error, TEXT("SaveTempSceneGraph: Failed to serialize JSON"));
        return false;
    }
    
    if (!FFileHelper::SaveStringToFile(JsonString, *TempSceneGraphPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogMAEditMode, Error, TEXT("SaveTempSceneGraph: Failed to write file: %s"), *TempSceneGraphPath);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("SaveTempSceneGraph: Saved to %s"), *TempSceneGraphPath);
    
    // 广播变化事件
    OnTempSceneGraphChanged.Broadcast();
    
    return true;
}

FString UMAEditModeManager::GenerateNextId()
{
    FString NewId = FString::Printf(TEXT("%d"), NextNodeId);
    NextNodeId++;
    return NewId;
}


//=============================================================================
// POI 管理
// Requirements: 3.1, 3.2, 3.4, 3.5
//=============================================================================

AMAPointOfInterest* UMAEditModeManager::CreatePOI(const FVector& WorldLocation)
{
    // Requirements: 3.1, 3.2
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreatePOI: World is null"));
        return nullptr;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    AMAPointOfInterest* POI = World->SpawnActor<AMAPointOfInterest>(
        AMAPointOfInterest::StaticClass(),
        WorldLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );
    
    if (POI)
    {
        // Requirements: 3.4 - POI 仅作为临时对象存在，不写入 Temp_Scene_Graph
        POIs.Add(POI);
        UE_LOG(LogMAEditMode, Log, TEXT("CreatePOI: Created POI at (%f, %f, %f)"), 
            WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
    }
    else
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreatePOI: Failed to spawn POI"));
    }
    
    return POI;
}

void UMAEditModeManager::DestroyPOI(AMAPointOfInterest* POI)
{
    if (!POI)
    {
        return;
    }
    
    // 从选择集合中移除
    SelectedPOIs.Remove(POI);
    
    // 从 POI 列表中移除
    POIs.Remove(POI);
    
    // 销毁 Actor
    POI->Destroy();
    
    UE_LOG(LogMAEditMode, Log, TEXT("DestroyPOI: Destroyed POI"));
}

void UMAEditModeManager::DestroyAllPOIs()
{
    // Requirements: 3.5
    
    // 清除选择
    SelectedPOIs.Empty();
    
    // 销毁所有 POI
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI && IsValid(POI))
        {
            POI->Destroy();
        }
    }
    
    POIs.Empty();
    
    UE_LOG(LogMAEditMode, Log, TEXT("DestroyAllPOIs: Destroyed all POIs"));
}

//=============================================================================
// 选择管理
// Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7
//=============================================================================

void UMAEditModeManager::SelectActor(AActor* Actor)
{
    // Requirements: 4.1, 4.5, 4.6
    
    if (!Actor)
    {
        return;
    }
    
    // Requirements: 4.6 - 互斥选择：选择 Actor 时清除 POI 选择
    if (SelectedPOIs.Num() > 0)
    {
        ClearPOIHighlight();
        SelectedPOIs.Empty();
    }
    
    // Requirements: 4.5 - Actor 单选：选中新 Actor 时自动取消之前的选择
    if (SelectedActor && SelectedActor != Actor)
    {
        ClearActorHighlight();
    }
    
    // 设置新选择
    SelectedActor = Actor;
    
    // Requirements: 4.3 - 高亮显示选中状态
    SetActorHighlight(Actor, true);
    
    UE_LOG(LogMAEditMode, Log, TEXT("SelectActor: Selected %s"), *Actor->GetName());
    
    // 广播选择变化
    OnSelectionChanged.Broadcast();
}

void UMAEditModeManager::SelectPOI(AMAPointOfInterest* POI)
{
    // Requirements: 4.2, 4.4, 4.6
    
    if (!POI)
    {
        return;
    }
    
    // Requirements: 4.6 - 互斥选择：选择 POI 时清除 Actor 选择
    if (SelectedActor)
    {
        ClearActorHighlight();
        SelectedActor = nullptr;
    }
    
    // Requirements: 4.4 - POI 多选
    if (!SelectedPOIs.Contains(POI))
    {
        SelectedPOIs.Add(POI);
        
        // Requirements: 4.3 - 高亮显示选中状态
        POI->SetHighlighted(true);
        
        UE_LOG(LogMAEditMode, Log, TEXT("SelectPOI: Added POI to selection, total=%d"), SelectedPOIs.Num());
    }
    
    // 广播选择变化
    OnSelectionChanged.Broadcast();
}

void UMAEditModeManager::DeselectObject(UObject* Object)
{
    // Requirements: 4.7
    
    if (!Object)
    {
        return;
    }
    
    // 检查是否是 Actor
    if (AActor* Actor = Cast<AActor>(Object))
    {
        if (SelectedActor == Actor)
        {
            ClearActorHighlight();
            SelectedActor = nullptr;
            UE_LOG(LogMAEditMode, Log, TEXT("DeselectObject: Deselected Actor %s"), *Actor->GetName());
            OnSelectionChanged.Broadcast();
        }
    }
    
    // 检查是否是 POI
    if (AMAPointOfInterest* POI = Cast<AMAPointOfInterest>(Object))
    {
        if (SelectedPOIs.Contains(POI))
        {
            POI->SetHighlighted(false);
            SelectedPOIs.Remove(POI);
            UE_LOG(LogMAEditMode, Log, TEXT("DeselectObject: Deselected POI, remaining=%d"), SelectedPOIs.Num());
            OnSelectionChanged.Broadcast();
        }
    }
}

void UMAEditModeManager::ClearSelection()
{
    // Requirements: 4.3
    
    // 清除 Actor 选择
    ClearActorHighlight();
    SelectedActor = nullptr;
    
    // 清除 POI 选择
    ClearPOIHighlight();
    SelectedPOIs.Empty();
    
    UE_LOG(LogMAEditMode, Log, TEXT("ClearSelection: Cleared all selections"));
    
    // 广播选择变化
    OnSelectionChanged.Broadcast();
}

void UMAEditModeManager::SetActorHighlight(AActor* Actor, bool bHighlight)
{
    if (!Actor)
    {
        return;
    }
    
    // 使用 Custom Depth Stencil 实现轮廓高亮
    TArray<UPrimitiveComponent*> PrimitiveComponents;
    Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
    
    for (UPrimitiveComponent* PrimComp : PrimitiveComponents)
    {
        if (PrimComp)
        {
            PrimComp->SetRenderCustomDepth(bHighlight);
            if (bHighlight)
            {
                PrimComp->SetCustomDepthStencilValue(1);
            }
        }
    }
}

void UMAEditModeManager::ClearActorHighlight()
{
    if (SelectedActor)
    {
        SetActorHighlight(SelectedActor, false);
    }
}

void UMAEditModeManager::ClearPOIHighlight()
{
    for (AMAPointOfInterest* POI : SelectedPOIs)
    {
        if (POI && IsValid(POI))
        {
            POI->SetHighlighted(false);
        }
    }
}

//=============================================================================
// Node 操作 (占位实现，将在后续任务中完善)
//=============================================================================

bool UMAEditModeManager::AddNode(const FString& NodeJson, FString& OutError)
{
    // 将在任务 4 中实现
    OutError = TEXT("Not implemented yet");
    return false;
}

bool UMAEditModeManager::DeleteNode(const FString& NodeId, FString& OutError)
{
    // 将在任务 4 中实现
    OutError = TEXT("Not implemented yet");
    return false;
}

bool UMAEditModeManager::EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
{
    // 将在任务 4 中实现
    OutError = TEXT("Not implemented yet");
    return false;
}

bool UMAEditModeManager::CreateGoal(const FVector& Location, const FString& Description, FString& OutError)
{
    // 将在任务 4 中实现
    OutError = TEXT("Not implemented yet");
    return false;
}

bool UMAEditModeManager::CreateZone(const TArray<FVector>& Vertices, const FString& Description, FString& OutError)
{
    // 将在任务 4 中实现
    OutError = TEXT("Not implemented yet");
    return false;
}

//=============================================================================
// 后端通信 (占位实现，将在后续任务中完善)
//=============================================================================

void UMAEditModeManager::SendSceneChangeMessage(const FString& ChangeType, const FString& Payload)
{
    // 将在任务 6 中实现
    UE_LOG(LogMAEditMode, Log, TEXT("SendSceneChangeMessage: Type=%s (not implemented yet)"), *ChangeType);
}
