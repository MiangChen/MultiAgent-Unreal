// MAHUDSceneEditCoordinator.cpp
// HUD 场景编辑协调器实现

#include "MAHUDSceneEditCoordinator.h"
#include "../MAHUD.h"
#include "../../Core/MAUIManager.h"
#include "../../Mode/MAEditWidget.h"
#include "../../Mode/MAModifyWidget.h"
#include "../../Mode/MASceneListWidget.h"
#include "../../../Core/Manager/MAEditModeManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Core/Manager/ue_tools/MAUESceneApplier.h"
#include "../../../Environment/Utils/MAGoalActor.h"
#include "../../../Environment/Utils/MAPointOfInterest.h"
#include "../../../Environment/Utils/MAZoneActor.h"
#include "Engine/GameInstance.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAHUDSceneEditCoordinator, Log, All);

void FMAHUDSceneEditCoordinator::HandleModifyConfirmed(AMAHUD* HUD, AActor* Actor, const FString& LabelText) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnModifyConfirmed: Actor=%s, LabelText=%s"),
        Actor ? *Actor->GetName() : TEXT("null"), *LabelText);
    if (!Actor)
    {
        HUD->ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UGameInstance* GI = HUD->GetWorld() ? HUD->GetWorld()->GetGameInstance() : nullptr;
    if (!GI)
    {
        HUD->ShowNotification(TEXT("Save failed: Unable to get GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        HUD->ShowNotification(TEXT("Save failed: SceneGraphManager not found"), true);
        return;
    }

    UMAModifyWidget* ModifyWidget = HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr;

    FString Id;
    FString Type;
    FString ErrorMessage;
    if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
    {
        HUD->ShowNotification(ErrorMessage, true);
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActor(Actor);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    const FVector WorldLocation = Actor->GetActorLocation();
    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnModifyConfirmed: Actor location = (%f, %f, %f)"),
        WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
    FString GeneratedLabel;
    if (!SceneGraphManager->AddSceneNode(Id, Type, WorldLocation, Actor, GeneratedLabel, ErrorMessage))
    {
        if (ErrorMessage.Contains(TEXT("ID already exists")))
        {
            HUD->ShowNotification(ErrorMessage, false, true);
            if (ModifyWidget)
            {
                ModifyWidget->ClearSelection();
            }
            HUD->ClearHighlightedActor();
        }
        else
        {
            HUD->ShowNotification(ErrorMessage, true);
            if (ModifyWidget)
            {
                ModifyWidget->SetSelectedActor(Actor);
                ModifyWidget->SetLabelText(LabelText);
            }
        }
        return;
    }

    const FString SuccessMessage = FString::Printf(TEXT("Label saved: %s"), *GeneratedLabel);
    HUD->ShowNotification(SuccessMessage, false);
    HUD->LoadSceneGraphForVisualization();
    if (ModifyWidget)
    {
        ModifyWidget->ClearSelection();
    }

    HUD->ClearHighlightedActor();
    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnModifyConfirmed: Successfully saved node with label=%s"), *GeneratedLabel);
}

void FMAHUDSceneEditCoordinator::HandleModifyActorSelected(AMAHUD* HUD, AActor* SelectedActor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnModifyActorSelected: Actor=%s"),
        SelectedActor ? *SelectedActor->GetName() : TEXT("null"));

    UMAModifyWidget* ModifyWidget = HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr;
    if (ModifyWidget)
    {
        ModifyWidget->SetSelectedActor(SelectedActor);
    }
}

void FMAHUDSceneEditCoordinator::HandleModifyActorsSelected(AMAHUD* HUD, const TArray<AActor*>& SelectedActors) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnModifyActorsSelected: %d actors"), SelectedActors.Num());

    UMAModifyWidget* ModifyWidget = HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr;
    if (ModifyWidget)
    {
        ModifyWidget->SetSelectedActors(SelectedActors);
    }
}

void FMAHUDSceneEditCoordinator::HandleMultiSelectModifyConfirmed(
    AMAHUD* HUD,
    const TArray<AActor*>& Actors,
    const FString& LabelText,
    const FString& GeneratedJson) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnMultiSelectModifyConfirmed: %d actors, LabelText=%s"),
        Actors.Num(), *LabelText);
    if (Actors.Num() == 0)
    {
        HUD->ShowNotification(TEXT("Please select at least one Actor first"), true);
        return;
    }

    UGameInstance* GI = HUD->GetWorld() ? HUD->GetWorld()->GetGameInstance() : nullptr;
    if (!GI)
    {
        HUD->ShowNotification(TEXT("Save failed: Unable to get GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        HUD->ShowNotification(TEXT("Save failed: SceneGraphManager not found"), true);
        return;
    }

    UMAModifyWidget* ModifyWidget = HUD->UIManager ? HUD->UIManager->GetModifyWidget() : nullptr;
    FString ErrorMessage;

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(GeneratedJson);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        HUD->ShowNotification(TEXT("JSON parse failed"), true);
        if (ModifyWidget)
        {
            ModifyWidget->SetSelectedActors(Actors);
            ModifyWidget->SetLabelText(LabelText);
        }
        return;
    }

    bool bIsEditMode = false;
    FString EditNodeId;
    if (JsonObject->TryGetBoolField(TEXT("_edit_mode"), bIsEditMode) && bIsEditMode)
    {
        JsonObject->TryGetStringField(TEXT("_edit_node_id"), EditNodeId);

        JsonObject->RemoveField(TEXT("_edit_mode"));
        JsonObject->RemoveField(TEXT("_edit_node_id"));

        FString CleanJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&CleanJson);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

        UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnMultiSelectModifyConfirmed: Edit mode, updating node %s"), *EditNodeId);

        if (!SceneGraphManager->EditNode(EditNodeId, CleanJson, ErrorMessage))
        {
            HUD->ShowNotification(ErrorMessage, true);
            if (ModifyWidget)
            {
                ModifyWidget->SetSelectedActors(Actors);
                ModifyWidget->SetLabelText(LabelText);
            }
            return;
        }

        const FMASceneGraphNode UpdatedNode = SceneGraphManager->GetNodeById(EditNodeId);
        if (UpdatedNode.IsValid())
        {
            FMAUESceneApplier::ApplyNodeToScene(HUD->GetWorld(), UpdatedNode);
        }

        SceneGraphManager->SaveToSource();
        HUD->ShowNotification(FString::Printf(TEXT("Node %s updated"), *EditNodeId), false);
    }
    else
    {
        FString Id;
        FString Type;
        if (!SceneGraphManager->ParseLabelInput(LabelText, Id, Type, ErrorMessage))
        {
            HUD->ShowNotification(ErrorMessage, true);
            if (ModifyWidget)
            {
                ModifyWidget->SetSelectedActors(Actors);
                ModifyWidget->SetLabelText(LabelText);
            }
            return;
        }

        if (SceneGraphManager->IsIdExists(Id))
        {
            HUD->ShowNotification(FString::Printf(TEXT("ID '%s' already exists, please use a different ID"), *Id), false, true);
            if (ModifyWidget)
            {
                ModifyWidget->SetSelectedActors(Actors);
                ModifyWidget->SetLabelText(LabelText);
            }
            return;
        }

        UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnMultiSelectModifyConfirmed: Add mode, saving JSON:\n%s"), *GeneratedJson);

        if (!SceneGraphManager->AddNode(GeneratedJson, ErrorMessage))
        {
            HUD->ShowNotification(ErrorMessage, true);
            if (ModifyWidget)
            {
                ModifyWidget->SetSelectedActors(Actors);
                ModifyWidget->SetLabelText(LabelText);
            }
            return;
        }

        SceneGraphManager->SaveToSource();
        HUD->ShowNotification(FString::Printf(TEXT("Multi-select annotation saved: %d Actors"), Actors.Num()), false);
    }

    HUD->LoadSceneGraphForVisualization();
    if (ModifyWidget)
    {
        ModifyWidget->ClearSelection();
    }
    HUD->ClearHighlightedActor();

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnMultiSelectModifyConfirmed: Successfully processed annotation"));
}

void FMAHUDSceneEditCoordinator::HandleEditConfirmed(AMAHUD* HUD, AActor* Actor, const FString& JsonContent) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditConfirmed: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    if (!Actor)
    {
        HUD->ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        HUD->ShowNotification(TEXT("Edit failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        HUD->ShowNotification(TEXT("Edit failed: EditModeManager not found"), true);
        return;
    }

    UGameInstance* GI = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        HUD->ShowNotification(TEXT("Edit failed: SceneGraphManager not found"), true);
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        HUD->ShowNotification(TEXT("Invalid JSON format"), true);
        return;
    }

    FString NodeId;
    if (!JsonObject->TryGetStringField(TEXT("id"), NodeId))
    {
        HUD->ShowNotification(TEXT("Missing id field in JSON"), true);
        return;
    }

    FString OutError;
    if (!SceneGraphManager->EditNode(NodeId, JsonContent, OutError))
    {
        HUD->ShowNotification(OutError, true);
        return;
    }

    const FMASceneGraphNode UpdatedNode = SceneGraphManager->GetNodeById(NodeId);
    if (UpdatedNode.IsValid())
    {
        FMAUESceneApplier::ApplyNodeToScene(World, UpdatedNode);
    }

    SceneGraphManager->SaveToSource();
    HUD->ShowNotification(TEXT("Changes saved"), false);
    EditModeManager->ClearSelection();
}

void FMAHUDSceneEditCoordinator::HandleEditDeleteActor(AMAHUD* HUD, AActor* Actor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditDeleteActor: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    if (!Actor)
    {
        HUD->ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        HUD->ShowNotification(TEXT("Delete failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        HUD->ShowNotification(TEXT("Delete failed: EditModeManager not found"), true);
        return;
    }

    UGameInstance* GI = World->GetGameInstance();
    UMASceneGraphManager* SceneGraphManager = GI ? GI->GetSubsystem<UMASceneGraphManager>() : nullptr;
    if (!SceneGraphManager)
    {
        HUD->ShowNotification(TEXT("Delete failed: SceneGraphManager not found"), true);
        return;
    }

    FString NodeId;
    bool bIsGoalOrZone = false;
    if (AMAGoalActor* GoalActor = Cast<AMAGoalActor>(Actor))
    {
        NodeId = GoalActor->GetNodeId();
        bIsGoalOrZone = true;
        UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditDeleteActor: GoalActor detected, NodeId=%s"), *NodeId);
    }
    else if (AMAZoneActor* ZoneActor = Cast<AMAZoneActor>(Actor))
    {
        NodeId = ZoneActor->GetNodeId();
        bIsGoalOrZone = true;
        UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditDeleteActor: ZoneActor detected, NodeId=%s"), *NodeId);
    }

    if (bIsGoalOrZone)
    {
        if (NodeId.IsEmpty())
        {
            HUD->ShowNotification(TEXT("Delete failed: Unable to get Node ID"), true);
            return;
        }

        FString OutError;
        if (!SceneGraphManager->DeleteNode(NodeId, OutError))
        {
            HUD->ShowNotification(OutError, true);
            return;
        }

        SceneGraphManager->SaveToSource();

        if (Actor->IsA<AMAGoalActor>())
        {
            EditModeManager->DestroyGoalActor(NodeId);
        }
        else if (Actor->IsA<AMAZoneActor>())
        {
            EditModeManager->DestroyZoneActor(NodeId);
        }

        HUD->ShowNotification(TEXT("Deleted"), false);
        EditModeManager->ClearSelection();
        return;
    }

    const FString ActorGuid = Actor->GetActorGuid().ToString();
    const TArray<FMASceneGraphNode> MatchingNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);
    if (MatchingNodes.Num() == 0)
    {
        HUD->ShowNotification(TEXT("No matching scene graph node found"), true);
        return;
    }

    FString OutError;
    if (!SceneGraphManager->DeleteNode(MatchingNodes[0].Id, OutError))
    {
        HUD->ShowNotification(OutError, true);
        return;
    }

    SceneGraphManager->SaveToSource();
    Actor->Destroy();
    HUD->ShowNotification(TEXT("Actor deleted"), false);
    EditModeManager->ClearSelection();
}

void FMAHUDSceneEditCoordinator::HandleEditCreateGoal(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& Description) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditCreateGoal: POI=%s, Description=%s"),
        POI ? *POI->GetName() : TEXT("null"), *Description);

    if (!POI)
    {
        HUD->ShowNotification(TEXT("Please select a POI first"), true);
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        HUD->ShowNotification(TEXT("Create failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        HUD->ShowNotification(TEXT("Create failed: EditModeManager not found"), true);
        return;
    }

    const FVector Location = POI->GetActorLocation();
    FString OutError;
    if (!EditModeManager->CreateGoal(Location, Description, OutError))
    {
        HUD->ShowNotification(OutError, true);
        return;
    }

    EditModeManager->DestroyPOI(POI);
    HUD->ShowNotification(TEXT("Goal created"), false);
    EditModeManager->ClearSelection();
}

void FMAHUDSceneEditCoordinator::HandleEditCreateZone(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs, const FString& Description) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditCreateZone: %d POIs, Description=%s"), POIs.Num(), *Description);

    if (POIs.Num() < 3)
    {
        HUD->ShowNotification(TEXT("Creating a zone requires at least 3 POIs"), true);
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        HUD->ShowNotification(TEXT("Create failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        HUD->ShowNotification(TEXT("Create failed: EditModeManager not found"), true);
        return;
    }

    TArray<FVector> Vertices;
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            Vertices.Add(POI->GetActorLocation());
        }
    }

    if (Vertices.Num() < 3)
    {
        HUD->ShowNotification(TEXT("Insufficient valid POIs"), true);
        return;
    }

    FString OutError;
    if (!EditModeManager->CreateZone(Vertices, Description, OutError))
    {
        HUD->ShowNotification(OutError, true);
        return;
    }

    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            EditModeManager->DestroyPOI(POI);
        }
    }

    HUD->ShowNotification(TEXT("Zone created"), false);
    EditModeManager->ClearSelection();
}

void FMAHUDSceneEditCoordinator::HandleEditAddPresetActor(AMAHUD* HUD, AMAPointOfInterest* POI, const FString& ActorType) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditAddPresetActor: POI=%s, ActorType=%s"),
        POI ? *POI->GetName() : TEXT("null"), *ActorType);

    if (!POI)
    {
        HUD->ShowNotification(TEXT("Please select a POI first"), true);
        return;
    }

    if (ActorType.IsEmpty() || ActorType == TEXT("(No preset Actors)"))
    {
        HUD->ShowNotification(TEXT("Please select a valid preset Actor type"), true);
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        HUD->ShowNotification(TEXT("Add failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        HUD->ShowNotification(TEXT("Add failed: EditModeManager not found"), true);
        return;
    }

    HUD->ShowNotification(TEXT("Preset Actor feature not yet implemented"), false, true);
    UE_LOG(LogMAHUDSceneEditCoordinator, Warning, TEXT("OnEditAddPresetActor: Preset Actor feature not yet implemented"));
}

void FMAHUDSceneEditCoordinator::HandleEditDeletePOIs(AMAHUD* HUD, const TArray<AMAPointOfInterest*>& POIs) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditDeletePOIs: %d POIs to delete"), POIs.Num());

    if (POIs.Num() == 0)
    {
        HUD->ShowNotification(TEXT("Please select POI first"), true);
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        HUD->ShowNotification(TEXT("Delete failed: Unable to get World"), true);
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        HUD->ShowNotification(TEXT("Delete failed: EditModeManager not found"), true);
        return;
    }

    int32 DeletedCount = 0;
    for (AMAPointOfInterest* POI : POIs)
    {
        if (POI)
        {
            EditModeManager->DestroyPOI(POI);
            DeletedCount++;
        }
    }

    HUD->ShowNotification(FString::Printf(TEXT("Deleted %d POIs"), DeletedCount), false);
    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditDeletePOIs: Deleted %d POIs"), DeletedCount);
}

void FMAHUDSceneEditCoordinator::HandleEditSetAsGoal(AMAHUD* HUD, AActor* Actor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditSetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    if (!Actor)
    {
        HUD->ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        HUD->ShowNotification(TEXT("Set failed: Unable to get World"), true);
        return;
    }

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        HUD->ShowNotification(TEXT("Set failed: Unable to get GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        HUD->ShowNotification(TEXT("Set failed: SceneGraphManager not found"), true);
        return;
    }

    const FString ActorGuid = Actor->GetActorGuid().ToString();
    const TArray<FMASceneGraphNode> MatchingNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);
    if (MatchingNodes.Num() == 0)
    {
        HUD->ShowNotification(TEXT("No matching scene graph node found"), true);
        return;
    }

    FString OutError;
    if (!SceneGraphManager->SetNodeAsGoal(MatchingNodes[0].Id, OutError))
    {
        HUD->ShowNotification(OutError, true);
        return;
    }

    FString NodeLabel = MatchingNodes[0].Label;
    if (NodeLabel.IsEmpty())
    {
        NodeLabel = MatchingNodes[0].Id;
    }

    HUD->ShowNotification(FString::Printf(TEXT("Set %s as Goal"), *NodeLabel), false);

    UMASceneListWidget* SceneListWidget = HUD->UIManager ? HUD->UIManager->GetSceneListWidget() : nullptr;
    if (SceneListWidget)
    {
        SceneListWidget->RefreshLists();
    }

    UMAEditWidget* EditWidget = HUD->UIManager ? HUD->UIManager->GetEditWidget() : nullptr;
    if (EditWidget)
    {
        EditWidget->SetSelectedActor(Actor);
    }
}

void FMAHUDSceneEditCoordinator::HandleEditUnsetAsGoal(AMAHUD* HUD, AActor* Actor) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnEditUnsetAsGoal: Actor=%s"), Actor ? *Actor->GetName() : TEXT("null"));

    if (!Actor)
    {
        HUD->ShowNotification(TEXT("Please select an Actor first"), true);
        return;
    }

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        HUD->ShowNotification(TEXT("Unset failed: Unable to get World"), true);
        return;
    }

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        HUD->ShowNotification(TEXT("Unset failed: Unable to get GameInstance"), true);
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        HUD->ShowNotification(TEXT("Unset failed: SceneGraphManager not found"), true);
        return;
    }

    const FString ActorGuid = Actor->GetActorGuid().ToString();
    const TArray<FMASceneGraphNode> MatchingNodes = SceneGraphManager->FindNodesByGuid(ActorGuid);
    if (MatchingNodes.Num() == 0)
    {
        HUD->ShowNotification(TEXT("No matching scene graph node found"), true);
        return;
    }

    FString NodeLabel = MatchingNodes[0].Label;
    if (NodeLabel.IsEmpty())
    {
        NodeLabel = MatchingNodes[0].Id;
    }

    FString OutError;
    if (!SceneGraphManager->UnsetNodeAsGoal(MatchingNodes[0].Id, OutError))
    {
        HUD->ShowNotification(OutError, true);
        return;
    }

    HUD->ShowNotification(FString::Printf(TEXT("Unset %s as Goal"), *NodeLabel), false);

    UMASceneListWidget* SceneListWidget = HUD->UIManager ? HUD->UIManager->GetSceneListWidget() : nullptr;
    if (SceneListWidget)
    {
        SceneListWidget->RefreshLists();
    }

    UMAEditWidget* EditWidget = HUD->UIManager ? HUD->UIManager->GetEditWidget() : nullptr;
    if (EditWidget)
    {
        EditWidget->SetSelectedActor(Actor);
    }
}

void FMAHUDSceneEditCoordinator::HandleSceneListGoalClicked(AMAHUD* HUD, const FString& GoalId) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnSceneListGoalClicked: GoalId=%s"), *GoalId);

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    AMAGoalActor* GoalActor = EditModeManager->GetGoalActorByNodeId(GoalId);
    if (GoalActor)
    {
        EditModeManager->SelectActor(GoalActor);
        UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnSceneListGoalClicked: Selected Goal Actor for %s"), *GoalId);
        return;
    }

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        return;
    }

    UMASceneGraphManager* SceneGraphManager = GI->GetSubsystem<UMASceneGraphManager>();
    if (!SceneGraphManager)
    {
        return;
    }

    const FMASceneGraphNode Node = SceneGraphManager->GetNodeById(GoalId);
    if (Node.IsValid() && !Node.Guid.IsEmpty())
    {
        AActor* FoundActor = EditModeManager->FindActorByGuid(Node.Guid);
        if (FoundActor)
        {
            EditModeManager->SelectActor(FoundActor);
            UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnSceneListGoalClicked: Selected Actor %s for Goal %s"),
                *FoundActor->GetName(), *GoalId);
        }
    }
}

void FMAHUDSceneEditCoordinator::HandleSceneListZoneClicked(AMAHUD* HUD, const FString& ZoneId) const
{
    if (!HUD)
    {
        return;
    }

    UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnSceneListZoneClicked: ZoneId=%s"), *ZoneId);

    UWorld* World = HUD->GetWorld();
    if (!World)
    {
        return;
    }

    UMAEditModeManager* EditModeManager = World->GetSubsystem<UMAEditModeManager>();
    if (!EditModeManager)
    {
        return;
    }

    AMAZoneActor* ZoneActor = EditModeManager->GetZoneActorByNodeId(ZoneId);
    if (ZoneActor)
    {
        EditModeManager->SelectActor(ZoneActor);
        UE_LOG(LogMAHUDSceneEditCoordinator, Log, TEXT("OnSceneListZoneClicked: Selected Zone Actor for %s"), *ZoneId);
    }
    else
    {
        UE_LOG(LogMAHUDSceneEditCoordinator, Warning, TEXT("OnSceneListZoneClicked: No Zone Actor found for %s"), *ZoneId);
        HUD->ShowNotification(FString::Printf(TEXT("Zone %s has no visualization Actor"), *ZoneId), false, true);
    }
}
