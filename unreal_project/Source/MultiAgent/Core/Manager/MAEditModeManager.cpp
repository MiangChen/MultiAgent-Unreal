// MAEditModeManager.cpp
// Edit Mode 管理器实现
// - 图操作 (AddNode/DeleteNode/EditNode) 委托给 MASceneGraphManager
// - 本类专注于 POI 管理、选择管理、Goal/Zone Actor 可视化管理

#include "MAEditModeManager.h"
#include "MASceneGraphManager.h"
#include "../Config/MAConfigManager.h"
#include "../../Agent/Skill/Utils/MAUESceneQuery.h"
#include "../../Environment/Utils/MAPointOfInterest.h"
#include "../../Environment/Utils/MAZoneActor.h"
#include "../../Environment/Utils/MAGoalActor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY(LogMAEditMode);

//=============================================================================
// 生命周期
//=============================================================================

void UMAEditModeManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogMAEditMode, Log, TEXT("MAEditModeManager initializing"));
    
    // 初始化 NextNodeId (从 MASceneGraphManager 获取最大 ID)
    if (UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager())
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphMgr->GetAllNodes();
        for (const FMASceneGraphNode& Node : AllNodes)
        {
            int32 IdNum = FCString::Atoi(*Node.Id);
            if (IdNum >= NextNodeId)
            {
                NextNodeId = IdNum + 1;
            }
        }
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("MAEditModeManager initialized, NextNodeId=%d"), NextNodeId);
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
    
    Super::Deinitialize();
}

//=============================================================================
// 内部辅助方法
//=============================================================================

UMASceneGraphManager* UMAEditModeManager::GetSceneGraphManager() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }
    
    return GameInstance->GetSubsystem<UMASceneGraphManager>();
}

FString UMAEditModeManager::GenerateNextId()
{
    FString NewId = FString::Printf(TEXT("%d"), NextNodeId);
    NextNodeId++;
    return NewId;
}

//=============================================================================
// POI 管理
//=============================================================================

AMAPointOfInterest* UMAEditModeManager::CreatePOI(const FVector& WorldLocation)
{
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
    
    SelectedPOIs.Remove(POI);
    POIs.Remove(POI);
    POI->Destroy();
    
    UE_LOG(LogMAEditMode, Log, TEXT("DestroyPOI: Destroyed POI"));
}

void UMAEditModeManager::DestroyAllPOIs()
{
    SelectedPOIs.Empty();
    
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
//=============================================================================

void UMAEditModeManager::SelectActor(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }
    
    if (SelectedPOIs.Num() > 0)
    {
        ClearPOIHighlight();
        SelectedPOIs.Empty();
    }
    
    if (SelectedActor && SelectedActor != Actor)
    {
        ClearActorHighlight();
    }
    
    SelectedActor = Actor;
    SetActorHighlight(Actor, true);
    
    UE_LOG(LogMAEditMode, Log, TEXT("SelectActor: Selected %s"), *Actor->GetName());
    OnSelectionChanged.Broadcast();
}

void UMAEditModeManager::SelectPOI(AMAPointOfInterest* POI)
{
    if (!POI)
    {
        return;
    }
    
    if (SelectedActor)
    {
        ClearActorHighlight();
        SelectedActor = nullptr;
    }
    
    if (!SelectedPOIs.Contains(POI))
    {
        SelectedPOIs.Add(POI);
        POI->SetHighlighted(true);
        UE_LOG(LogMAEditMode, Log, TEXT("SelectPOI: Added POI to selection, total=%d"), SelectedPOIs.Num());
    }
    
    OnSelectionChanged.Broadcast();
}

void UMAEditModeManager::DeselectObject(UObject* Object)
{
    if (!Object)
    {
        return;
    }
    
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
    ClearActorHighlight();
    SelectedActor = nullptr;
    
    ClearPOIHighlight();
    SelectedPOIs.Empty();
    
    UE_LOG(LogMAEditMode, Log, TEXT("ClearSelection: Cleared all selections"));
    OnSelectionChanged.Broadcast();
}


//=============================================================================
// 高亮管理辅助函数
//=============================================================================

static AActor* FindRootActorForEdit(AActor* Actor)
{
    if (!Actor) return nullptr;
    
    AActor* Current = Actor;
    AActor* Parent = Current->GetAttachParentActor();
    
    while (Parent)
    {
        Current = Parent;
        Parent = Current->GetAttachParentActor();
    }
    
    return Current;
}

static void CollectActorAndChildrenForEdit(AActor* Actor, TArray<AActor*>& OutActors)
{
    if (!Actor) return;
    
    OutActors.Add(Actor);
    
    TArray<AActor*> AttachedActors;
    Actor->GetAttachedActors(AttachedActors);
    
    for (AActor* Child : AttachedActors)
    {
        CollectActorAndChildrenForEdit(Child, OutActors);
    }
}

static void SetSingleActorHighlightForEdit(AActor* Actor, bool bHighlight)
{
    if (!Actor) return;
    
    TArray<UPrimitiveComponent*> Components;
    Actor->GetComponents<UPrimitiveComponent>(Components);
    
    FLinearColor HighlightColor(0.3f, 0.6f, 1.0f);
    
    for (UPrimitiveComponent* Comp : Components)
    {
        if (Comp)
        {
            Comp->SetRenderCustomDepth(bHighlight);
            Comp->SetCustomDepthStencilValue(bHighlight ? 2 : 0);
            
            UMeshComponent* MeshComp = Cast<UMeshComponent>(Comp);
            if (MeshComp)
            {
                if (bHighlight)
                {
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
    
    AActor* RootActor = FindRootActorForEdit(Actor);
    
    TArray<AActor*> AllActors;
    CollectActorAndChildrenForEdit(RootActor, AllActors);
    
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
// Goal/Zone 创建 (委托给 MASceneGraphManager)
//=============================================================================

bool UMAEditModeManager::CreateGoal(const FVector& Location, const FString& Description, FString& OutError)
{
    UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager();
    if (!SceneGraphMgr)
    {
        OutError = TEXT("MASceneGraphManager not available");
        UE_LOG(LogMAEditMode, Error, TEXT("CreateGoal: %s"), *OutError);
        return false;
    }
    
    // 生成新的 ID
    FString GoalId = FString::Printf(TEXT("goal_%s"), *GenerateNextId());
    
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
    
    // 委托给 MASceneGraphManager::AddNode()
    FString AddError;
    if (!SceneGraphMgr->AddNode(GoalJson, AddError))
    {
        OutError = FString::Printf(TEXT("Failed to add Goal node: %s"), *AddError);
        UE_LOG(LogMAEditMode, Error, TEXT("CreateGoal: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("CreateGoal: Created Goal node %s at (%f, %f, %f)"),
        *GoalId, Location.X, Location.Y, Location.Z);
    
    // 创建视觉 Actor
    CreateGoalActor(GoalId, Location, Description);
    
    return true;
}

bool UMAEditModeManager::CreateZone(const TArray<FVector>& Vertices, const FString& Description, FString& OutError)
{
    UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager();
    if (!SceneGraphMgr)
    {
        OutError = TEXT("MASceneGraphManager not available");
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
    
    // 计算凸包
    TArray<FVector> ConvexHull = ComputeConvexHull(Vertices);
    
    if (ConvexHull.Num() < 3)
    {
        OutError = TEXT("Failed to compute convex hull (points may be collinear)");
        UE_LOG(LogMAEditMode, Error, TEXT("CreateZone: %s"), *OutError);
        return false;
    }
    
    // 生成新的 ID
    FString ZoneId = FString::Printf(TEXT("zone_%s"), *GenerateNextId());
    
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
    
    // 委托给 MASceneGraphManager::AddNode()
    FString AddError;
    if (!SceneGraphMgr->AddNode(ZoneJson, AddError))
    {
        OutError = FString::Printf(TEXT("Failed to add Zone node: %s"), *AddError);
        UE_LOG(LogMAEditMode, Error, TEXT("CreateZone: %s"), *OutError);
        return false;
    }
    
    UE_LOG(LogMAEditMode, Log, TEXT("CreateZone: Created Zone node %s with %d vertices"),
        *ZoneId, ConvexHull.Num());
    
    // 创建视觉 Actor
    CreateZoneActor(ZoneId, ConvexHull);
    
    return true;
}


//=============================================================================
// Zone/Goal Actor 管理
//=============================================================================

AMAZoneActor* UMAEditModeManager::CreateZoneActor(const FString& NodeId, const TArray<FVector>& Vertices)
{
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
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogMAEditMode, Error, TEXT("CreateGoalActor: World is null"));
        return nullptr;
    }
    
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
// 凸包计算
//=============================================================================

TArray<FVector> UMAEditModeManager::ComputeConvexHull(const TArray<FVector>& Points) const
{
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
    if (SortedPoints.Num() > 2)
    {
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
                    double DistA = VA.SizeSquared();
                    double DistB = VB.SizeSquared();
                    bShouldSwap = DistA > DistB;
                }
                else
                {
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
        if (UniquePoints.Num() >= 2)
        {
            FVector& Last = UniquePoints.Last();
            FVector2D VA(Last.X - Pivot.X, Last.Y - Pivot.Y);
            FVector2D VB(SortedPoints[i].X - Pivot.X, SortedPoints[i].Y - Pivot.Y);
            
            double Cross = VA.X * VB.Y - VA.Y * VB.X;
            
            if (FMath::Abs(Cross) < KINDA_SMALL_NUMBER)
            {
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
        while (Stack.Num() > 1)
        {
            FVector Top = Stack.Last();
            FVector SecondTop = Stack[Stack.Num() - 2];
            
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
// Edit Mode 状态
//=============================================================================

bool UMAEditModeManager::IsEditModeAvailable() const
{
    // Edit Mode 需要 MASceneGraphManager 已初始化
    UMASceneGraphManager* SceneGraphMgr = GetSceneGraphManager();
    if (!SceneGraphMgr)
    {
        return false;
    }
    
    // 检查场景图文件是否存在
    FString FilePath = SceneGraphMgr->GetSceneGraphFilePath();
    return FPaths::FileExists(FilePath);
}

AActor* UMAEditModeManager::FindActorByGuid(const FString& Guid) const
{
    UWorld* World = GetWorld();
    return FMAUESceneQuery::FindActorByGuid(World, Guid);
}
