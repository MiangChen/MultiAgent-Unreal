// MAEditModeManager.cpp
// Edit Mode 管理器实现
// Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 3.1, 3.2, 3.4, 3.5, 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7
// Requirements: 6.4, 7.4, 8.4, 9.6, 10.7, 11.1

#include "MAEditModeManager.h"
#include "MACommSubsystem.h"
#include "../Environment/MAPointOfInterest.h"
#include "../Environment/MAZoneActor.h"
#include "../Environment/MAGoalActor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"  // For TActorIterator

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
    
    // 销毁所有 Zone Actor
    DestroyAllZoneActors();
    
    // 销毁所有 Goal Actor
    DestroyAllGoalActors();
    
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
    
    // 保存到临时文件
    if (!FFileHelper::SaveStringToFile(JsonString, *TempSceneGraphPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogMAEditMode, Error, TEXT("SaveTempSceneGraph: Failed to write file: %s"), *TempSceneGraphPath);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("SaveTempSceneGraph: Saved to %s"), *TempSceneGraphPath);
    
    // 同时保存到 datasets/scene_graph_cyberworld_temp.json
    FString SyncFilePath = FPaths::ProjectDir() / TEXT("datasets/scene_graph_cyberworld_temp.json");
    if (!FFileHelper::SaveStringToFile(JsonString, *SyncFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("SaveTempSceneGraph: Failed to sync to %s"), *SyncFilePath);
    }
    else
    {
        UE_LOG(LogMAEditMode, Log, TEXT("SaveTempSceneGraph: Synced to %s"), *SyncFilePath);
    }
    
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

// 查找根 Actor（向上遍历 Attach 父级直到找到最顶层）
static AActor* FindRootActorForEdit(AActor* Actor)
{
    if (!Actor) return nullptr;
    
    AActor* Current = Actor;
    AActor* Parent = Current->GetAttachParentActor();
    
    // 向上遍历直到没有父 Actor
    while (Parent)
    {
        Current = Parent;
        Parent = Current->GetAttachParentActor();
    }
    
    return Current;
}

// 递归收集 Actor 及其所有子 Actor
static void CollectActorAndChildrenForEdit(AActor* Actor, TArray<AActor*>& OutActors)
{
    if (!Actor) return;
    
    OutActors.Add(Actor);
    
    // 获取所有附加的子 Actor
    TArray<AActor*> AttachedActors;
    Actor->GetAttachedActors(AttachedActors);
    
    for (AActor* Child : AttachedActors)
    {
        CollectActorAndChildrenForEdit(Child, OutActors);
    }
}

// 对单个 Actor 设置高亮（不递归）
static void SetSingleActorHighlightForEdit(AActor* Actor, bool bHighlight)
{
    if (!Actor) return;
    
    // 获取所有 PrimitiveComponent
    TArray<UPrimitiveComponent*> Components;
    Actor->GetComponents<UPrimitiveComponent>(Components);
    
    // 高亮颜色 - 使用蓝色区分于 Modify 模式的青绿色
    FLinearColor HighlightColor(0.3f, 0.6f, 1.0f);  // 蓝色
    
    for (UPrimitiveComponent* Comp : Components)
    {
        if (Comp)
        {
            // 方式 1: Custom Depth (需要 Post Process 配合)
            Comp->SetRenderCustomDepth(bHighlight);
            Comp->SetCustomDepthStencilValue(bHighlight ? 2 : 0);  // 使用 2 区分于 Modify 模式
            
            // 方式 2: 使用 Overlay Material 实现高亮
            UMeshComponent* MeshComp = Cast<UMeshComponent>(Comp);
            if (MeshComp)
            {
                if (bHighlight)
                {
                    // 创建高亮材质
                    static UMaterial* UnlitMaterial = nullptr;
                    if (!UnlitMaterial)
                    {
                        UnlitMaterial = LoadObject<UMaterial>(nullptr, 
                            TEXT("/Engine/EngineMaterials/DefaultDeferredDecalMaterial.DefaultDeferredDecalMaterial"));
                        
                        if (!UnlitMaterial)
                        {
                            UnlitMaterial = LoadObject<UMaterial>(nullptr, 
                                TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
                        }
                    }
                    
                    if (UnlitMaterial)
                    {
                        UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(UnlitMaterial, MeshComp);
                        if (DynMat)
                        {
                            DynMat->SetVectorParameterValue(TEXT("BaseColor"), HighlightColor);
                            DynMat->SetVectorParameterValue(TEXT("Color"), HighlightColor);
                            MeshComp->SetOverlayMaterial(DynMat);
                        }
                    }
                }
                else
                {
                    // 清除 Overlay Material
                    MeshComp->SetOverlayMaterial(nullptr);
                }
            }
        }
    }
}

void UMAEditModeManager::SetActorHighlight(AActor* Actor, bool bHighlight)
{
    if (!Actor)
    {
        return;
    }
    
    // 查找根 Actor
    AActor* RootActor = FindRootActorForEdit(Actor);
    
    // 收集根 Actor 及其所有子 Actor
    TArray<AActor*> AllActors;
    CollectActorAndChildrenForEdit(RootActor, AllActors);
    
    // 对所有 Actor 设置高亮
    for (AActor* ActorToHighlight : AllActors)
    {
        SetSingleActorHighlightForEdit(ActorToHighlight, bHighlight);
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("SetActorHighlight: %s highlight for %s (Root: %s, Total: %d actors)"), 
        bHighlight ? TEXT("Enabled") : TEXT("Disabled"), 
        *Actor->GetName(),
        *RootActor->GetName(),
        AllActors.Num());
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
// Node 操作
// Requirements: 6.3, 7.2, 7.3, 8.3, 9.3, 9.4, 9.5, 10.3, 10.4, 10.5, 10.6
//=============================================================================

bool UMAEditModeManager::IsPointTypeNode(const TSharedPtr<FJsonObject>& NodeObject) const
{
    // Requirements: 5.3, 5.5, 7.5, 12.5
    if (!NodeObject.IsValid())
    {
        return false;
    }
    
    const TSharedPtr<FJsonObject>* ShapeObject;
    if (!NodeObject->TryGetObjectField(TEXT("shape"), ShapeObject) || !ShapeObject->IsValid())
    {
        return false;
    }
    
    FString ShapeType;
    if (!(*ShapeObject)->TryGetStringField(TEXT("type"), ShapeType))
    {
        return false;
    }
    
    return ShapeType.Equals(TEXT("point"), ESearchCase::IgnoreCase);
}

TSharedPtr<FJsonObject> UMAEditModeManager::FindNodeById(const FString& NodeId) const
{
    if (!TempSceneGraphData.IsValid() || NodeId.IsEmpty())
    {
        return nullptr;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (!TempSceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        return nullptr;
    }
    
    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        if (NodeValue.IsValid() && NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
            FString Id;
            if (NodeObject->TryGetStringField(TEXT("id"), Id) && Id == NodeId)
            {
                return NodeObject;
            }
        }
    }
    
    return nullptr;
}

FString UMAEditModeManager::GetNodeJsonById(const FString& NodeId) const
{
    TSharedPtr<FJsonObject> NodeObject = FindNodeById(NodeId);
    if (!NodeObject.IsValid())
    {
        return FString();
    }
    
    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    if (FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer))
    {
        return JsonString;
    }
    
    return FString();
}

int32 UMAEditModeManager::FindNodeIndexById(const FString& NodeId) const
{
    if (!TempSceneGraphData.IsValid() || NodeId.IsEmpty())
    {
        return -1;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (!TempSceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        return -1;
    }
    
    for (int32 i = 0; i < NodesArray->Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& NodeValue = (*NodesArray)[i];
        if (NodeValue.IsValid() && NodeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
            FString Id;
            if (NodeObject->TryGetStringField(TEXT("id"), Id) && Id == NodeId)
            {
                return i;
            }
        }
    }
    
    return -1;
}

int32 UMAEditModeManager::DeleteEdgesForNode(const FString& NodeId)
{
    // Requirements: 7.3 - 自动删除与该 Node 相连的所有 Edge
    
    if (!TempSceneGraphData.IsValid() || NodeId.IsEmpty())
    {
        return 0;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* EdgesArrayConst;
    if (!TempSceneGraphData->TryGetArrayField(TEXT("edges"), EdgesArrayConst))
    {
        return 0;
    }
    
    // 复制数组以便修改
    TArray<TSharedPtr<FJsonValue>> EdgesArray = *EdgesArrayConst;
    int32 DeletedCount = 0;
    
    // 从后向前遍历，以便安全删除
    for (int32 i = EdgesArray.Num() - 1; i >= 0; --i)
    {
        const TSharedPtr<FJsonValue>& EdgeValue = EdgesArray[i];
        if (EdgeValue.IsValid() && EdgeValue->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> EdgeObject = EdgeValue->AsObject();
            FString Source, Target;
            EdgeObject->TryGetStringField(TEXT("source"), Source);
            EdgeObject->TryGetStringField(TEXT("target"), Target);
            
            if (Source == NodeId || Target == NodeId)
            {
                EdgesArray.RemoveAt(i);
                DeletedCount++;
                UE_LOG(LogMAEditMode, Log, TEXT("DeleteEdgesForNode: Deleted edge %s -> %s"), *Source, *Target);
            }
        }
    }
    
    // 将修改后的数组设置回去
    if (DeletedCount > 0)
    {
        TempSceneGraphData->SetArrayField(TEXT("edges"), EdgesArray);
    }
    
    return DeletedCount;
}

AActor* UMAEditModeManager::FindActorByGuid(const FString& Guid) const
{
    if (Guid.IsEmpty())
    {
        return nullptr;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    // 规范化输入的 GUID（移除连字符，转为大写）
    FString NormalizedInputGuid = Guid.Replace(TEXT("-"), TEXT("")).ToUpper();
    
    // 遍历所有 Actor 查找匹配的 GUID
    // 使用 Actor->GetActorGuid().ToString() 获取 GUID（与 MASceneGraphManager 保持一致）
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (Actor)
        {
            // 使用 UE5 内置的 Actor GUID
            FString ActorGuid = Actor->GetActorGuid().ToString();
            
            // 规范化 Actor 的 GUID（移除连字符，转为大写）
            FString NormalizedActorGuid = ActorGuid.Replace(TEXT("-"), TEXT("")).ToUpper();
            
            if (NormalizedActorGuid == NormalizedInputGuid)
            {
                return Actor;
            }
        }
    }
    
    return nullptr;
}

TArray<FString> UMAEditModeManager::FindNodeIdsByGuid(const FString& Guid) const
{
    TArray<FString> Result;
    
    if (Guid.IsEmpty() || !TempSceneGraphData.IsValid())
    {
        return Result;
    }
    
    // 规范化输入的 GUID（移除连字符，转为大写）
    FString NormalizedInputGuid = Guid.Replace(TEXT("-"), TEXT("")).ToUpper();
    
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (!TempSceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        return Result;
    }
    
    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        if (!NodeValue.IsValid() || NodeValue->Type != EJson::Object)
        {
            continue;
        }
        
        TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
        if (!NodeObject.IsValid())
        {
            continue;
        }
        
        FString NodeId;
        if (!NodeObject->TryGetStringField(TEXT("id"), NodeId))
        {
            continue;
        }
        
        // 检查单个 guid 字段 (point 类型)
        FString NodeGuid;
        if (NodeObject->TryGetStringField(TEXT("guid"), NodeGuid))
        {
            FString NormalizedNodeGuid = NodeGuid.Replace(TEXT("-"), TEXT("")).ToUpper();
            if (NormalizedNodeGuid == NormalizedInputGuid)
            {
                Result.Add(NodeId);
                continue;
            }
        }
        
        // 检查 Guid 数组 (polygon/linestring 类型)
        const TArray<TSharedPtr<FJsonValue>>* GuidArray;
        if (NodeObject->TryGetArrayField(TEXT("Guid"), GuidArray))
        {
            for (const TSharedPtr<FJsonValue>& GuidValue : *GuidArray)
            {
                if (GuidValue.IsValid())
                {
                    FString ArrayGuid = GuidValue->AsString();
                    FString NormalizedArrayGuid = ArrayGuid.Replace(TEXT("-"), TEXT("")).ToUpper();
                    if (NormalizedArrayGuid == NormalizedInputGuid)
                    {
                        Result.Add(NodeId);
                        break;
                    }
                }
            }
        }
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("FindNodeIdsByGuid: Found %d nodes for GUID %s"), Result.Num(), *Guid);
    return Result;
}

void UMAEditModeManager::SyncActorPositionFromNode(const TSharedPtr<FJsonObject>& NodeObject)
{
    // Requirements: 6.5, 12.2, 12.3 - 同步更新虚幻场景中 Actor 的位置
    
    if (!NodeObject.IsValid())
    {
        return;
    }
    
    // 只处理 point 类型
    if (!IsPointTypeNode(NodeObject))
    {
        return;
    }
    
    // 获取 GUID
    FString Guid;
    if (!NodeObject->TryGetStringField(TEXT("guid"), Guid))
    {
        // 尝试获取 Guid 数组的第一个元素
        const TArray<TSharedPtr<FJsonValue>>* GuidArray;
        if (NodeObject->TryGetArrayField(TEXT("Guid"), GuidArray) && GuidArray->Num() > 0)
        {
            Guid = (*GuidArray)[0]->AsString();
        }
    }
    
    if (Guid.IsEmpty())
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("SyncActorPositionFromNode: No GUID found"));
        return;
    }
    
    // 获取新位置
    const TSharedPtr<FJsonObject>* ShapeObject;
    if (!NodeObject->TryGetObjectField(TEXT("shape"), ShapeObject))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* CenterArray;
    if (!(*ShapeObject)->TryGetArrayField(TEXT("center"), CenterArray) || CenterArray->Num() < 3)
    {
        return;
    }
    
    FVector NewLocation(
        (*CenterArray)[0]->AsNumber(),
        (*CenterArray)[1]->AsNumber(),
        (*CenterArray)[2]->AsNumber()
    );
    
    // 查找并更新 Actor 位置
    AActor* Actor = FindActorByGuid(Guid);
    if (Actor)
    {
        Actor->SetActorLocation(NewLocation);
        UE_LOG(LogMAEditMode, Log, TEXT("SyncActorPositionFromNode: Updated Actor %s to (%f, %f, %f)"),
            *Actor->GetName(), NewLocation.X, NewLocation.Y, NewLocation.Z);
    }
    else
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("SyncActorPositionFromNode: Actor with GUID %s not found"), *Guid);
    }
}

bool UMAEditModeManager::AddNode(const FString& NodeJson, FString& OutError)
{
    // Requirements: 8.3 - 在 Temp_Scene_Graph 中创建对应的 Node
    
    if (!TempSceneGraphData.IsValid())
    {
        OutError = TEXT("Temp scene graph not loaded");
        UE_LOG(LogMAEditMode, Error, TEXT("AddNode: %s"), *OutError);
        return false;
    }
    
    // 解析输入的 JSON
    TSharedPtr<FJsonObject> NewNodeObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodeJson);
    if (!FJsonSerializer::Deserialize(Reader, NewNodeObject) || !NewNodeObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format");
        UE_LOG(LogMAEditMode, Error, TEXT("AddNode: %s"), *OutError);
        return false;
    }
    
    // 如果没有 ID，生成一个新的
    FString NodeId;
    if (!NewNodeObject->TryGetStringField(TEXT("id"), NodeId) || NodeId.IsEmpty())
    {
        NodeId = GenerateNextId();
        NewNodeObject->SetStringField(TEXT("id"), NodeId);
    }
    else
    {
        // 检查 ID 是否已存在
        if (FindNodeById(NodeId).IsValid())
        {
            OutError = FString::Printf(TEXT("Node with ID %s already exists"), *NodeId);
            UE_LOG(LogMAEditMode, Error, TEXT("AddNode: %s"), *OutError);
            return false;
        }
    }
    
    // 获取 nodes 数组
    const TArray<TSharedPtr<FJsonValue>>* NodesArrayConst;
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    
    if (TempSceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArrayConst))
    {
        NodesArray = *NodesArrayConst;
    }
    
    // 添加新 Node
    NodesArray.Add(MakeShareable(new FJsonValueObject(NewNodeObject)));
    
    // 将修改后的数组设置回去
    TempSceneGraphData->SetArrayField(TEXT("nodes"), NodesArray);
    
    // 保存临时场景图
    if (!SaveTempSceneGraph())
    {
        OutError = TEXT("Failed to save temp scene graph");
        UE_LOG(LogMAEditMode, Error, TEXT("AddNode: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("AddNode: Added node with ID %s"), *NodeId);
    
    // 发送后端通知
    SendSceneChangeMessage(TEXT("add_node"), NodeJson);
    
    return true;
}

bool UMAEditModeManager::DeleteNode(const FString& NodeId, FString& OutError)
{
    // Requirements: 7.2, 7.3, 7.5 - 从 Temp_Scene_Graph 中删除对应的 Node
    
    if (!TempSceneGraphData.IsValid())
    {
        OutError = TEXT("Temp scene graph not loaded");
        UE_LOG(LogMAEditMode, Error, TEXT("DeleteNode: %s"), *OutError);
        return false;
    }
    
    // 查找 Node
    TSharedPtr<FJsonObject> NodeObject = FindNodeById(NodeId);
    if (!NodeObject.IsValid())
    {
        OutError = FString::Printf(TEXT("Node with ID %s not found"), *NodeId);
        UE_LOG(LogMAEditMode, Error, TEXT("DeleteNode: %s"), *OutError);
        return false;
    }
    
    // 检查是否为 Goal 或 Zone 类型 (这些始终可删除)
    bool bIsGoalOrZone = false;
    bool bHasIsGoalFlag = false;  // 普通 node 是否有 is_goal: true
    FString NodeType;
    const TSharedPtr<FJsonObject>* PropertiesObject;
    if (NodeObject->TryGetObjectField(TEXT("properties"), PropertiesObject) && PropertiesObject->IsValid())
    {
        if ((*PropertiesObject)->TryGetStringField(TEXT("type"), NodeType))
        {
            bIsGoalOrZone = NodeType.Equals(TEXT("goal"), ESearchCase::IgnoreCase) || 
                           NodeType.Equals(TEXT("zone"), ESearchCase::IgnoreCase);
        }
        
        // 检查普通 node 是否有 is_goal: true
        if (!bIsGoalOrZone)
        {
            bool bIsGoal = false;
            if ((*PropertiesObject)->TryGetBoolField(TEXT("is_goal"), bIsGoal) && bIsGoal)
            {
                bHasIsGoalFlag = true;
            }
        }
    }
    
    // Requirements: 7.5 - 仅允许删除 shape 类型为 point 的 Node，或 Goal/Zone 类型
    if (!bIsGoalOrZone && !IsPointTypeNode(NodeObject))
    {
        OutError = TEXT("Only point type nodes or Goal/Zone can be deleted");
        UE_LOG(LogMAEditMode, Error, TEXT("DeleteNode: %s"), *OutError);
        return false;
    }
    
    // 查找 Node 索引
    int32 NodeIndex = FindNodeIndexById(NodeId);
    if (NodeIndex < 0)
    {
        OutError = FString::Printf(TEXT("Node index not found for ID %s"), *NodeId);
        UE_LOG(LogMAEditMode, Error, TEXT("DeleteNode: %s"), *OutError);
        return false;
    }
    
    // Requirements: 7.3 - 自动删除与该 Node 相连的所有 Edge
    int32 DeletedEdges = DeleteEdgesForNode(NodeId);
    UE_LOG(LogMAEditMode, Log, TEXT("DeleteNode: Deleted %d edges for node %s"), DeletedEdges, *NodeId);
    
    // 删除 Node
    const TArray<TSharedPtr<FJsonValue>>* NodesArrayConst;
    if (TempSceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArrayConst))
    {
        TArray<TSharedPtr<FJsonValue>> NodesArray = *NodesArrayConst;
        NodesArray.RemoveAt(NodeIndex);
        TempSceneGraphData->SetArrayField(TEXT("nodes"), NodesArray);
    }
    
    // 保存临时场景图
    if (!SaveTempSceneGraph())
    {
        OutError = TEXT("Failed to save temp scene graph");
        UE_LOG(LogMAEditMode, Error, TEXT("DeleteNode: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("DeleteNode: Deleted node with ID %s"), *NodeId);
    
    // 发送后端通知
    FString Payload = FString::Printf(TEXT("{\"id\":\"%s\"}"), *NodeId);
    SendSceneChangeMessage(TEXT("delete_node"), Payload);
    
    // 如果是 Goal 或 Zone 类型，还需要发送额外的 delete_goal 或 delete_zone 消息
    if (bIsGoalOrZone)
    {
        if (NodeType.Equals(TEXT("goal"), ESearchCase::IgnoreCase))
        {
            SendSceneChangeMessage(TEXT("delete_goal"), Payload);
            UE_LOG(LogMAEditMode, Log, TEXT("DeleteNode: Sent delete_goal for Goal type node %s"), *NodeId);
        }
        else if (NodeType.Equals(TEXT("zone"), ESearchCase::IgnoreCase))
        {
            SendSceneChangeMessage(TEXT("delete_zone"), Payload);
            UE_LOG(LogMAEditMode, Log, TEXT("DeleteNode: Sent delete_zone for Zone type node %s"), *NodeId);
        }
    }
    // 如果是普通 node 但有 is_goal: true，也要发送 delete_goal
    else if (bHasIsGoalFlag)
    {
        SendSceneChangeMessage(TEXT("delete_goal"), Payload);
        UE_LOG(LogMAEditMode, Log, TEXT("DeleteNode: Sent delete_goal for node %s with is_goal flag"), *NodeId);
    }
    
    return true;
}

bool UMAEditModeManager::EditNode(const FString& NodeId, const FString& NewNodeJson, FString& OutError)
{
    // Requirements: 6.3, 5.3, 5.5, 12.5 - 更新 Temp_Scene_Graph 中对应的 Node
    
    if (!TempSceneGraphData.IsValid())
    {
        OutError = TEXT("Temp scene graph not loaded");
        UE_LOG(LogMAEditMode, Error, TEXT("EditNode: %s"), *OutError);
        return false;
    }
    
    // 查找现有 Node
    TSharedPtr<FJsonObject> ExistingNode = FindNodeById(NodeId);
    if (!ExistingNode.IsValid())
    {
        OutError = FString::Printf(TEXT("Node with ID %s not found"), *NodeId);
        UE_LOG(LogMAEditMode, Error, TEXT("EditNode: %s"), *OutError);
        return false;
    }
    
    // 解析新的 JSON
    TSharedPtr<FJsonObject> NewNodeObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NewNodeJson);
    if (!FJsonSerializer::Deserialize(Reader, NewNodeObject) || !NewNodeObject.IsValid())
    {
        OutError = TEXT("Invalid JSON format");
        UE_LOG(LogMAEditMode, Error, TEXT("EditNode: %s"), *OutError);
        return false;
    }
    
    // 检查 shape 类型编辑约束
    bool bIsPointType = IsPointTypeNode(ExistingNode);
    
    // Requirements: 5.3, 5.5, 12.5 - 根据 shape 类型限制可编辑字段
    if (!bIsPointType)
    {
        // polygon, linestring, prism 类型仅允许编辑 properties
        // 直接使用原始节点的 shape 数据，避免大型 JSON 序列化比较导致的性能问题和崩溃
        
        // 深拷贝原始 shape 对象（通过序列化再反序列化）
        const TSharedPtr<FJsonObject>* ExistingShapePtr;
        if (ExistingNode->TryGetObjectField(TEXT("shape"), ExistingShapePtr) && ExistingShapePtr->IsValid())
        {
            // 序列化原始 shape 为 JSON 字符串
            FString ShapeJsonStr;
            TSharedRef<TJsonWriter<>> ShapeWriter = TJsonWriterFactory<>::Create(&ShapeJsonStr);
            if (FJsonSerializer::Serialize((*ExistingShapePtr).ToSharedRef(), ShapeWriter))
            {
                // 反序列化为新的 shape 对象（深拷贝）
                TSharedPtr<FJsonObject> ShapeCopy;
                TSharedRef<TJsonReader<>> ShapeReader = TJsonReaderFactory<>::Create(ShapeJsonStr);
                if (FJsonSerializer::Deserialize(ShapeReader, ShapeCopy) && ShapeCopy.IsValid())
                {
                    NewNodeObject->SetObjectField(TEXT("shape"), ShapeCopy);
                    UE_LOG(LogMAEditMode, Log, TEXT("EditNode: Non-point type node, preserving original shape data (deep copy)"));
                }
            }
        }
        
        // 同样保留原始的 Guid 数组（polygon/linestring 类型使用 Guid 数组而非单个 guid）
        const TArray<TSharedPtr<FJsonValue>>* ExistingGuidArray;
        if (ExistingNode->TryGetArrayField(TEXT("Guid"), ExistingGuidArray))
        {
            // 深拷贝 Guid 数组
            TArray<TSharedPtr<FJsonValue>> GuidArrayCopy;
            for (const TSharedPtr<FJsonValue>& GuidValue : *ExistingGuidArray)
            {
                if (GuidValue.IsValid())
                {
                    GuidArrayCopy.Add(MakeShareable(new FJsonValueString(GuidValue->AsString())));
                }
            }
            NewNodeObject->SetArrayField(TEXT("Guid"), GuidArrayCopy);
        }
        
        // 保留原始的 count 字段
        int32 ExistingCount;
        if (ExistingNode->TryGetNumberField(TEXT("count"), ExistingCount))
        {
            NewNodeObject->SetNumberField(TEXT("count"), ExistingCount);
        }
    }
    
    // 确保 ID 不变
    NewNodeObject->SetStringField(TEXT("id"), NodeId);
    
    // 查找 Node 索引并替换
    int32 NodeIndex = FindNodeIndexById(NodeId);
    if (NodeIndex < 0)
    {
        OutError = FString::Printf(TEXT("Node index not found for ID %s"), *NodeId);
        UE_LOG(LogMAEditMode, Error, TEXT("EditNode: %s"), *OutError);
        return false;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* NodesArrayConst;
    if (TempSceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArrayConst))
    {
        TArray<TSharedPtr<FJsonValue>> NodesArray = *NodesArrayConst;
        NodesArray[NodeIndex] = MakeShareable(new FJsonValueObject(NewNodeObject));
        TempSceneGraphData->SetArrayField(TEXT("nodes"), NodesArray);
    }
    
    // Requirements: 6.5, 12.2, 12.3 - 如果是 point 类型且修改了 shape.center，同步更新 Actor 位置
    if (bIsPointType)
    {
        SyncActorPositionFromNode(NewNodeObject);
    }
    
    // 保存临时场景图
    if (!SaveTempSceneGraph())
    {
        OutError = TEXT("Failed to save temp scene graph");
        UE_LOG(LogMAEditMode, Error, TEXT("EditNode: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("EditNode: Updated node with ID %s"), *NodeId);
    
    // 发送后端通知
    SendSceneChangeMessage(TEXT("edit_node"), NewNodeJson);
    
    return true;
}

bool UMAEditModeManager::CreateGoal(const FVector& Location, const FString& Description, FString& OutError)
{
    // Requirements: 9.3, 9.4, 9.5, 9.6, 9.7 - 在 Temp_Scene_Graph 中创建 Goal Node
    
    if (!TempSceneGraphData.IsValid())
    {
        OutError = TEXT("Temp scene graph not loaded");
        UE_LOG(LogMAEditMode, Error, TEXT("CreateGoal: %s"), *OutError);
        return false;
    }
    
    // 生成新的 ID
    FString GoalId = FString::Printf(TEXT("goal_%s"), *GenerateNextId());
    
    // Requirements: 9.4, 9.5 - 设置正确的属性
    // 构建 Goal Node JSON
    TSharedPtr<FJsonObject> GoalNode = MakeShareable(new FJsonObject());
    GoalNode->SetStringField(TEXT("id"), GoalId);
    
    // 生成 GUID
    FGuid NewGuid = FGuid::NewGuid();
    GoalNode->SetStringField(TEXT("guid"), NewGuid.ToString(EGuidFormats::DigitsWithHyphens));
    
    // Properties
    TSharedPtr<FJsonObject> Properties = MakeShareable(new FJsonObject());
    Properties->SetStringField(TEXT("type"), TEXT("goal"));
    Properties->SetStringField(TEXT("label"), FString::Printf(TEXT("Goal-%s"), *GoalId));
    Properties->SetStringField(TEXT("category"), TEXT("goal"));
    Properties->SetStringField(TEXT("status"), TEXT("uncompleted"));
    Properties->SetStringField(TEXT("description"), Description);
    GoalNode->SetObjectField(TEXT("properties"), Properties);
    
    // Shape - point type
    TSharedPtr<FJsonObject> Shape = MakeShareable(new FJsonObject());
    Shape->SetStringField(TEXT("type"), TEXT("point"));
    
    TArray<TSharedPtr<FJsonValue>> CenterArray;
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.X)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
    CenterArray.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
    Shape->SetArrayField(TEXT("center"), CenterArray);
    
    GoalNode->SetObjectField(TEXT("shape"), Shape);
    
    // 序列化为 JSON 字符串
    FString GoalJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&GoalJson);
    if (!FJsonSerializer::Serialize(GoalNode.ToSharedRef(), Writer))
    {
        OutError = TEXT("Failed to serialize Goal node");
        UE_LOG(LogMAEditMode, Error, TEXT("CreateGoal: %s"), *OutError);
        return false;
    }
    
    // 添加到场景图
    FString AddError;
    if (!AddNode(GoalJson, AddError))
    {
        OutError = FString::Printf(TEXT("Failed to add Goal node: %s"), *AddError);
        UE_LOG(LogMAEditMode, Error, TEXT("CreateGoal: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("CreateGoal: Created Goal node %s at (%f, %f, %f)"),
        *GoalId, Location.X, Location.Y, Location.Z);
    
    // Requirements: 15.2 - 创建 Goal Actor 可视化
    CreateGoalActor(GoalId, Location, Description);
    
    // Requirements: 9.6 - 发送 add_goal 消息通知后端
    SendSceneChangeMessage(TEXT("add_goal"), GoalJson);
    
    return true;
}

bool UMAEditModeManager::CreateZone(const TArray<FVector>& Vertices, const FString& Description, FString& OutError)
{
    // Requirements: 10.3, 10.4, 10.5, 10.6, 10.7, 10.8 - 根据 POI 点创建 Zone Node
    
    if (!TempSceneGraphData.IsValid())
    {
        OutError = TEXT("Temp scene graph not loaded");
        UE_LOG(LogMAEditMode, Error, TEXT("CreateZone: %s"), *OutError);
        return false;
    }
    
    // 至少需要 3 个点
    if (Vertices.Num() < 3)
    {
        OutError = TEXT("At least 3 vertices required to create a zone");
        UE_LOG(LogMAEditMode, Error, TEXT("CreateZone: %s"), *OutError);
        return false;
    }
    
    // Requirements: 10.3 - 计算凸包作为 Zone 边界
    TArray<FVector> ConvexHull = ComputeConvexHull(Vertices);
    
    if (ConvexHull.Num() < 3)
    {
        OutError = TEXT("Failed to compute convex hull (points may be collinear)");
        UE_LOG(LogMAEditMode, Error, TEXT("CreateZone: %s"), *OutError);
        return false;
    }
    
    // 生成新的 ID
    FString ZoneId = FString::Printf(TEXT("zone_%s"), *GenerateNextId());
    
    // Requirements: 10.5, 10.6 - 设置正确的属性
    // 构建 Zone Node JSON
    TSharedPtr<FJsonObject> ZoneNode = MakeShareable(new FJsonObject());
    ZoneNode->SetStringField(TEXT("id"), ZoneId);
    
    // 生成 GUID
    FGuid NewGuid = FGuid::NewGuid();
    ZoneNode->SetStringField(TEXT("guid"), NewGuid.ToString(EGuidFormats::DigitsWithHyphens));
    
    // Properties
    TSharedPtr<FJsonObject> Properties = MakeShareable(new FJsonObject());
    Properties->SetStringField(TEXT("type"), TEXT("zone"));
    Properties->SetStringField(TEXT("label"), FString::Printf(TEXT("Zone-%s"), *ZoneId));
    Properties->SetStringField(TEXT("category"), TEXT("zone"));
    Properties->SetStringField(TEXT("status"), TEXT("undiscovered"));
    Properties->SetStringField(TEXT("description"), Description);
    ZoneNode->SetObjectField(TEXT("properties"), Properties);
    
    // Shape - polygon type
    TSharedPtr<FJsonObject> Shape = MakeShareable(new FJsonObject());
    Shape->SetStringField(TEXT("type"), TEXT("polygon"));
    
    // 构建顶点数组
    TArray<TSharedPtr<FJsonValue>> VerticesArray;
    for (const FVector& Vertex : ConvexHull)
    {
        TArray<TSharedPtr<FJsonValue>> PointArray;
        PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.X)));
        PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.Y)));
        PointArray.Add(MakeShareable(new FJsonValueNumber(Vertex.Z)));
        VerticesArray.Add(MakeShareable(new FJsonValueArray(PointArray)));
    }
    Shape->SetArrayField(TEXT("vertices"), VerticesArray);
    
    ZoneNode->SetObjectField(TEXT("shape"), Shape);
    
    // 序列化为 JSON 字符串
    FString ZoneJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ZoneJson);
    if (!FJsonSerializer::Serialize(ZoneNode.ToSharedRef(), Writer))
    {
        OutError = TEXT("Failed to serialize Zone node");
        UE_LOG(LogMAEditMode, Error, TEXT("CreateZone: %s"), *OutError);
        return false;
    }
    
    // 添加到场景图
    FString AddError;
    if (!AddNode(ZoneJson, AddError))
    {
        OutError = FString::Printf(TEXT("Failed to add Zone node: %s"), *AddError);
        UE_LOG(LogMAEditMode, Error, TEXT("CreateZone: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("CreateZone: Created Zone node %s with %d vertices"),
        *ZoneId, ConvexHull.Num());
    
    // Requirements: 14.1 - 创建 Zone Actor 可视化
    CreateZoneActor(ZoneId, ConvexHull);
    
    // Requirements: 10.7 - 发送 add_zone 消息通知后端
    SendSceneChangeMessage(TEXT("add_zone"), ZoneJson);
    
    return true;
}

//=============================================================================
// 后端通信
// Requirements: 6.4, 7.4, 8.4, 9.6, 10.7, 11.1
//=============================================================================

void UMAEditModeManager::SendSceneChangeMessage(const FString& ChangeType, const FString& Payload)
{
    // Requirements: 11.1 - 通过 MACommSubsystem 发送场景变化消息
    
    UE_LOG(LogMAEditMode, Log, TEXT("SendSceneChangeMessage: Type=%s"), *ChangeType);
    UE_LOG(LogMAEditMode, Verbose, TEXT("Payload: %s"), *Payload);
    
    // 获取 GameInstance
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("SendSceneChangeMessage: World is null"));
        return;
    }
    
    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("SendSceneChangeMessage: GameInstance is null"));
        return;
    }
    
    // 获取 MACommSubsystem
    UMACommSubsystem* CommSubsystem = GameInstance->GetSubsystem<UMACommSubsystem>();
    if (!CommSubsystem)
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("SendSceneChangeMessage: MACommSubsystem not found"));
        return;
    }
    
    // 将字符串类型转换为枚举类型
    EMASceneChangeType ChangeTypeEnum = FMASceneChangeMessage::StringToChangeType(ChangeType);
    
    // 创建场景变化消息
    FMASceneChangeMessage Message(ChangeTypeEnum, Payload);
    
    // 发送消息
    CommSubsystem->SendSceneChangeMessage(Message);
    
    UE_LOG(LogMAEditMode, Log, TEXT("SendSceneChangeMessage: Message sent successfully"));
}

void UMAEditModeManager::SendSceneChangeMessageByType(EMASceneChangeType ChangeType, const FString& Payload)
{
    // Requirements: 11.1 - 使用枚举类型发送消息
    
    FString ChangeTypeStr = FMASceneChangeMessage::ChangeTypeToString(ChangeType);
    SendSceneChangeMessage(ChangeTypeStr, Payload);
}

//=============================================================================
// 辅助函数
//=============================================================================

TArray<FVector> UMAEditModeManager::ComputeConvexHull(const TArray<FVector>& Points) const
{
    // Requirements: 10.3 - 计算凸包
    // 使用 Graham Scan 算法，返回逆时针顺序的凸包顶点
    
    TArray<FVector> Result;
    
    if (Points.Num() < 3)
    {
        return Result;
    }
    
    // 复制点集用于排序
    TArray<FVector> SortedPoints = Points;
    
    // 找到最低点（Y 最小，如果相同则 X 最小）
    int32 LowestIndex = 0;
    for (int32 i = 1; i < SortedPoints.Num(); ++i)
    {
        if (SortedPoints[i].Y < SortedPoints[LowestIndex].Y ||
            (SortedPoints[i].Y == SortedPoints[LowestIndex].Y && SortedPoints[i].X < SortedPoints[LowestIndex].X))
        {
            LowestIndex = i;
        }
    }
    
    // 将最低点移到第一个位置
    Swap(SortedPoints[0], SortedPoints[LowestIndex]);
    FVector Pivot = SortedPoints[0];
    
    // 按极角排序（相对于 Pivot）
    // 从索引 1 开始排序
    if (SortedPoints.Num() > 2)
    {
        // 手动实现排序，避免 Lambda 问题
        for (int32 i = 1; i < SortedPoints.Num() - 1; ++i)
        {
            for (int32 j = i + 1; j < SortedPoints.Num(); ++j)
            {
                FVector2D VA(SortedPoints[i].X - Pivot.X, SortedPoints[i].Y - Pivot.Y);
                FVector2D VB(SortedPoints[j].X - Pivot.X, SortedPoints[j].Y - Pivot.Y);
                
                double Cross = VA.X * VB.Y - VA.Y * VB.X;
                
                bool bShouldSwap = false;
                if (FMath::Abs(Cross) < KINDA_SMALL_NUMBER)
                {
                    // 共线，按距离排序
                    double DistA = VA.SizeSquared();
                    double DistB = VB.SizeSquared();
                    bShouldSwap = DistA > DistB;
                }
                else
                {
                    // 逆时针方向
                    bShouldSwap = Cross < 0;
                }
                
                if (bShouldSwap)
                {
                    Swap(SortedPoints[i], SortedPoints[j]);
                }
            }
        }
    }
    
    // 移除共线的点（保留最远的）
    TArray<FVector> UniquePoints;
    UniquePoints.Add(SortedPoints[0]);
    
    for (int32 i = 1; i < SortedPoints.Num(); ++i)
    {
        // 检查是否与前一个点共线
        if (UniquePoints.Num() >= 2)
        {
            FVector& Last = UniquePoints.Last();
            FVector2D VA(Last.X - Pivot.X, Last.Y - Pivot.Y);
            FVector2D VB(SortedPoints[i].X - Pivot.X, SortedPoints[i].Y - Pivot.Y);
            
            double Cross = VA.X * VB.Y - VA.Y * VB.X;
            
            if (FMath::Abs(Cross) < KINDA_SMALL_NUMBER)
            {
                // 共线，保留更远的点
                double DistA = VA.SizeSquared();
                double DistB = VB.SizeSquared();
                if (DistB > DistA)
                {
                    UniquePoints.Last() = SortedPoints[i];
                }
                continue;
            }
        }
        UniquePoints.Add(SortedPoints[i]);
    }
    
    if (UniquePoints.Num() < 3)
    {
        return Result;
    }
    
    // Graham Scan
    TArray<FVector> Stack;
    Stack.Add(UniquePoints[0]);
    Stack.Add(UniquePoints[1]);
    
    for (int32 i = 2; i < UniquePoints.Num(); ++i)
    {
        // 移除不是左转的点
        while (Stack.Num() > 1)
        {
            FVector Top = Stack.Last();
            FVector SecondTop = Stack[Stack.Num() - 2];
            
            // 计算叉积
            FVector2D V1(Top.X - SecondTop.X, Top.Y - SecondTop.Y);
            FVector2D V2(UniquePoints[i].X - Top.X, UniquePoints[i].Y - Top.Y);
            
            double Cross = V1.X * V2.Y - V1.Y * V2.X;
            
            if (Cross <= 0)
            {
                Stack.Pop();
            }
            else
            {
                break;
            }
        }
        Stack.Add(UniquePoints[i]);
    }
    
    Result = Stack;
    
    UE_LOG(LogMAEditMode, Log, TEXT("ComputeConvexHull: Input %d points, Output %d vertices"),
        Points.Num(), Result.Num());
    
    return Result;
}


//=============================================================================
// Zone/Goal Actor 管理
// Requirements: 14.1, 14.6, 15.2, 15.6
//=============================================================================

AMAZoneActor* UMAEditModeManager::CreateZoneActor(const FString& NodeId, const TArray<FVector>& Vertices)
{
    // Requirements: 14.1 - 为 Zone Node 创建可视化 Actor
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreateZoneActor: World is null"));
        return nullptr;
    }
    
    if (Vertices.Num() < 3)
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("CreateZoneActor: Need at least 3 vertices"));
        return nullptr;
    }
    
    // 检查是否已存在
    if (ZoneActors.Contains(NodeId))
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("CreateZoneActor: Zone Actor for %s already exists"), *NodeId);
        return ZoneActors[NodeId];
    }
    
    // 计算中心位置
    FVector Center = FVector::ZeroVector;
    for (const FVector& V : Vertices)
    {
        Center += V;
    }
    Center /= Vertices.Num();
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    AMAZoneActor* ZoneActor = World->SpawnActor<AMAZoneActor>(
        AMAZoneActor::StaticClass(),
        Center,
        FRotator::ZeroRotator,
        SpawnParams
    );
    
    if (ZoneActor)
    {
        ZoneActor->SetNodeId(NodeId);
        ZoneActor->SetVertices(Vertices);
        ZoneActors.Add(NodeId, ZoneActor);
        
        UE_LOG(LogMAEditMode, Log, TEXT("CreateZoneActor: Created Zone Actor for %s with %d vertices"), 
            *NodeId, Vertices.Num());
    }
    else
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreateZoneActor: Failed to spawn Zone Actor"));
    }
    
    return ZoneActor;
}

void UMAEditModeManager::DestroyZoneActor(const FString& NodeId)
{
    if (AMAZoneActor** FoundActor = ZoneActors.Find(NodeId))
    {
        if (*FoundActor && IsValid(*FoundActor))
        {
            (*FoundActor)->Destroy();
        }
        ZoneActors.Remove(NodeId);
        UE_LOG(LogMAEditMode, Log, TEXT("DestroyZoneActor: Destroyed Zone Actor for %s"), *NodeId);
    }
}

void UMAEditModeManager::DestroyAllZoneActors()
{
    // Requirements: 14.6 - Edit Mode 退出时销毁所有 Zone Actor
    
    for (auto& Pair : ZoneActors)
    {
        if (Pair.Value && IsValid(Pair.Value))
        {
            Pair.Value->Destroy();
        }
    }
    ZoneActors.Empty();
    
    UE_LOG(LogMAEditMode, Log, TEXT("DestroyAllZoneActors: Destroyed all Zone Actors"));
}

AMAZoneActor* UMAEditModeManager::GetZoneActorByNodeId(const FString& NodeId) const
{
    if (const AMAZoneActor* const* FoundActor = ZoneActors.Find(NodeId))
    {
        return const_cast<AMAZoneActor*>(*FoundActor);
    }
    return nullptr;
}

AMAGoalActor* UMAEditModeManager::CreateGoalActor(const FString& NodeId, const FVector& Location, const FString& Description)
{
    // Requirements: 15.2 - 为 Goal Node 创建可视化 Actor
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreateGoalActor: World is null"));
        return nullptr;
    }
    
    // 检查是否已存在
    if (GoalActors.Contains(NodeId))
    {
        UE_LOG(LogMAEditMode, Warning, TEXT("CreateGoalActor: Goal Actor for %s already exists"), *NodeId);
        return GoalActors[NodeId];
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    AMAGoalActor* GoalActor = World->SpawnActor<AMAGoalActor>(
        AMAGoalActor::StaticClass(),
        Location,
        FRotator::ZeroRotator,
        SpawnParams
    );
    
    if (GoalActor)
    {
        GoalActor->SetNodeId(NodeId);
        GoalActor->SetDescription(Description);
        GoalActor->SetLabel(FString::Printf(TEXT("Goal-%s"), *NodeId));
        GoalActors.Add(NodeId, GoalActor);
        
        UE_LOG(LogMAEditMode, Log, TEXT("CreateGoalActor: Created Goal Actor for %s at (%f, %f, %f)"), 
            *NodeId, Location.X, Location.Y, Location.Z);
    }
    else
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreateGoalActor: Failed to spawn Goal Actor"));
    }
    
    return GoalActor;
}

void UMAEditModeManager::DestroyGoalActor(const FString& NodeId)
{
    if (AMAGoalActor** FoundActor = GoalActors.Find(NodeId))
    {
        if (*FoundActor && IsValid(*FoundActor))
        {
            (*FoundActor)->Destroy();
        }
        GoalActors.Remove(NodeId);
        UE_LOG(LogMAEditMode, Log, TEXT("DestroyGoalActor: Destroyed Goal Actor for %s"), *NodeId);
    }
}

void UMAEditModeManager::DestroyAllGoalActors()
{
    // Requirements: 15.6 - Edit Mode 退出时销毁所有 Goal Actor
    
    for (auto& Pair : GoalActors)
    {
        if (Pair.Value && IsValid(Pair.Value))
        {
            Pair.Value->Destroy();
        }
    }
    GoalActors.Empty();
    
    UE_LOG(LogMAEditMode, Log, TEXT("DestroyAllGoalActors: Destroyed all Goal Actors"));
}

AMAGoalActor* UMAEditModeManager::GetGoalActorByNodeId(const FString& NodeId) const
{
    if (const AMAGoalActor* const* FoundActor = GoalActors.Find(NodeId))
    {
        return const_cast<AMAGoalActor*>(*FoundActor);
    }
    return nullptr;
}

//=============================================================================
// 设为 Goal 功能
// Requirements: 16.1, 16.2, 16.3, 16.4, 16.5, 16.6
//=============================================================================

bool UMAEditModeManager::SetNodeAsGoal(const FString& NodeId, FString& OutError)
{
    // Requirements: 16.2, 16.3, 16.4 - 将 Node 设为 Goal
    
    if (!TempSceneGraphData.IsValid())
    {
        OutError = TEXT("Temp scene graph not loaded");
        UE_LOG(LogMAEditMode, Error, TEXT("SetNodeAsGoal: %s"), *OutError);
        return false;
    }
    
    // 查找 Node
    TSharedPtr<FJsonObject> NodeObject = FindNodeById(NodeId);
    if (!NodeObject.IsValid())
    {
        OutError = FString::Printf(TEXT("Node with ID %s not found"), *NodeId);
        UE_LOG(LogMAEditMode, Error, TEXT("SetNodeAsGoal: %s"), *OutError);
        return false;
    }
    
    // 获取或创建 properties 对象
    TSharedPtr<FJsonObject> Properties;
    const TSharedPtr<FJsonObject>* PropertiesPtr;
    if (NodeObject->TryGetObjectField(TEXT("properties"), PropertiesPtr))
    {
        Properties = *PropertiesPtr;
    }
    else
    {
        Properties = MakeShareable(new FJsonObject());
        NodeObject->SetObjectField(TEXT("properties"), Properties);
    }
    
    // 添加 is_goal: true
    Properties->SetBoolField(TEXT("is_goal"), true);
    
    // 保存临时场景图
    if (!SaveTempSceneGraph())
    {
        OutError = TEXT("Failed to save temp scene graph");
        UE_LOG(LogMAEditMode, Error, TEXT("SetNodeAsGoal: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("SetNodeAsGoal: Set node %s as goal"), *NodeId);
    
    // 序列化 Node 为 JSON 字符串
    FString NodeJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeJson);
    FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer);
    
    // 发送后端通知: edit_node + add_goal
    SendSceneChangeMessage(TEXT("edit_node"), NodeJson);
    SendSceneChangeMessage(TEXT("add_goal"), NodeJson);
    UE_LOG(LogMAEditMode, Log, TEXT("SetNodeAsGoal: Sent edit_node and add_goal for node %s"), *NodeId);
    
    return true;
}

bool UMAEditModeManager::UnsetNodeAsGoal(const FString& NodeId, FString& OutError)
{
    // Requirements: 16.6 - 取消 Node 的 Goal 状态
    
    if (!TempSceneGraphData.IsValid())
    {
        OutError = TEXT("Temp scene graph not loaded");
        UE_LOG(LogMAEditMode, Error, TEXT("UnsetNodeAsGoal: %s"), *OutError);
        return false;
    }
    
    // 查找 Node
    TSharedPtr<FJsonObject> NodeObject = FindNodeById(NodeId);
    if (!NodeObject.IsValid())
    {
        OutError = FString::Printf(TEXT("Node with ID %s not found"), *NodeId);
        UE_LOG(LogMAEditMode, Error, TEXT("UnsetNodeAsGoal: %s"), *OutError);
        return false;
    }
    
    // 获取 properties 对象
    const TSharedPtr<FJsonObject>* PropertiesPtr;
    if (!NodeObject->TryGetObjectField(TEXT("properties"), PropertiesPtr))
    {
        OutError = TEXT("Node has no properties");
        UE_LOG(LogMAEditMode, Warning, TEXT("UnsetNodeAsGoal: %s"), *OutError);
        return true;  // 没有 properties 也算成功
    }
    
    TSharedPtr<FJsonObject> Properties = *PropertiesPtr;
    
    // 移除 is_goal 字段
    Properties->RemoveField(TEXT("is_goal"));
    
    // 保存临时场景图
    if (!SaveTempSceneGraph())
    {
        OutError = TEXT("Failed to save temp scene graph");
        UE_LOG(LogMAEditMode, Error, TEXT("UnsetNodeAsGoal: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("UnsetNodeAsGoal: Unset node %s as goal"), *NodeId);
    
    // 序列化 Node 为 JSON 字符串
    FString NodeJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeJson);
    FJsonSerializer::Serialize(NodeObject.ToSharedRef(), Writer);
    
    // 发送后端通知: edit_node + delete_goal
    SendSceneChangeMessage(TEXT("edit_node"), NodeJson);
    FString Payload = FString::Printf(TEXT("{\"id\":\"%s\"}"), *NodeId);
    SendSceneChangeMessage(TEXT("delete_goal"), Payload);
    UE_LOG(LogMAEditMode, Log, TEXT("UnsetNodeAsGoal: Sent edit_node and delete_goal for node %s"), *NodeId);
    
    return true;
}

bool UMAEditModeManager::IsNodeGoal(const FString& NodeId) const
{
    // Requirements: 16.5 - 检查 Node 是否为 Goal
    
    if (!TempSceneGraphData.IsValid())
    {
        return false;
    }
    
    TSharedPtr<FJsonObject> NodeObject = FindNodeById(NodeId);
    if (!NodeObject.IsValid())
    {
        return false;
    }
    
    // 检查 properties.is_goal
    const TSharedPtr<FJsonObject>* PropertiesPtr;
    if (!NodeObject->TryGetObjectField(TEXT("properties"), PropertiesPtr))
    {
        return false;
    }
    
    bool bIsGoal = false;
    (*PropertiesPtr)->TryGetBoolField(TEXT("is_goal"), bIsGoal);
    
    return bIsGoal;
}

//=============================================================================
// 列表查询
// Requirements: 17.2, 17.3
//=============================================================================

TArray<FString> UMAEditModeManager::GetAllGoalNodeIds() const
{
    // Requirements: 17.2 - 获取所有 Goal Node ID
    
    TArray<FString> GoalIds;
    
    if (!TempSceneGraphData.IsValid())
    {
        return GoalIds;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (!TempSceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        return GoalIds;
    }
    
    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        if (!NodeValue.IsValid() || NodeValue->Type != EJson::Object)
        {
            continue;
        }
        
        TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
        
        // 检查 properties.type == "goal" 或 properties.is_goal == true
        const TSharedPtr<FJsonObject>* PropertiesPtr;
        if (NodeObject->TryGetObjectField(TEXT("properties"), PropertiesPtr))
        {
            FString Type;
            bool bIsGoal = false;
            
            (*PropertiesPtr)->TryGetStringField(TEXT("type"), Type);
            (*PropertiesPtr)->TryGetBoolField(TEXT("is_goal"), bIsGoal);
            
            if (Type.Equals(TEXT("goal"), ESearchCase::IgnoreCase) || bIsGoal)
            {
                FString NodeId;
                if (NodeObject->TryGetStringField(TEXT("id"), NodeId))
                {
                    GoalIds.Add(NodeId);
                }
            }
        }
    }
    
    return GoalIds;
}

TArray<FString> UMAEditModeManager::GetAllZoneNodeIds() const
{
    // Requirements: 17.3 - 获取所有 Zone Node ID
    
    TArray<FString> ZoneIds;
    
    if (!TempSceneGraphData.IsValid())
    {
        return ZoneIds;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (!TempSceneGraphData->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        return ZoneIds;
    }
    
    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        if (!NodeValue.IsValid() || NodeValue->Type != EJson::Object)
        {
            continue;
        }
        
        TSharedPtr<FJsonObject> NodeObject = NodeValue->AsObject();
        
        // 检查 properties.type == "zone" 或 shape.type == "polygon"
        const TSharedPtr<FJsonObject>* PropertiesPtr;
        const TSharedPtr<FJsonObject>* ShapePtr;
        
        bool bIsZone = false;
        
        if (NodeObject->TryGetObjectField(TEXT("properties"), PropertiesPtr))
        {
            FString Type;
            (*PropertiesPtr)->TryGetStringField(TEXT("type"), Type);
            if (Type.Equals(TEXT("zone"), ESearchCase::IgnoreCase))
            {
                bIsZone = true;
            }
        }
        
        // 也检查 shape.type == "polygon" (Zone 通常是多边形)
        if (!bIsZone && NodeObject->TryGetObjectField(TEXT("shape"), ShapePtr))
        {
            FString ShapeType;
            (*ShapePtr)->TryGetStringField(TEXT("type"), ShapeType);
            if (ShapeType.Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
            {
                // 检查 properties.category == "zone"
                if (PropertiesPtr)
                {
                    FString Category;
                    (*PropertiesPtr)->TryGetStringField(TEXT("category"), Category);
                    if (Category.Equals(TEXT("zone"), ESearchCase::IgnoreCase))
                    {
                        bIsZone = true;
                    }
                }
            }
        }
        
        if (bIsZone)
        {
            FString NodeId;
            if (NodeObject->TryGetStringField(TEXT("id"), NodeId))
            {
                ZoneIds.Add(NodeId);
            }
        }
    }
    
    return ZoneIds;
}

FString UMAEditModeManager::GetNodeLabel(const FString& NodeId) const
{
    if (!TempSceneGraphData.IsValid())
    {
        return FString();
    }
    
    TSharedPtr<FJsonObject> NodeObject = FindNodeById(NodeId);
    if (!NodeObject.IsValid())
    {
        return FString();
    }
    
    // 尝试从 properties.label 获取
    const TSharedPtr<FJsonObject>* PropertiesPtr;
    if (NodeObject->TryGetObjectField(TEXT("properties"), PropertiesPtr))
    {
        FString Label;
        if ((*PropertiesPtr)->TryGetStringField(TEXT("label"), Label))
        {
            return Label;
        }
    }
    
    // 回退到 Node ID
    return NodeId;
}
