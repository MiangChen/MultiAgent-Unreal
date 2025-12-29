// MASceneQuery.cpp

#include "MASceneQuery.h"
#include "MAGeometryUtils.h"
#include "../../../Environment/MAPickupItem.h"
#include "../../Character/MACharacter.h"
#include "Kismet/GameplayStatics.h"

FMASceneQueryResult FMASceneQuery::FindObjectByLabel(UWorld* World, const FMASemanticLabel& Label)
{
    FMASceneQueryResult Result;
    if (!World || Label.IsEmpty()) return Result;
    
    if (Label.IsGround())
    {
        Result.bFound = true;
        Result.Name = TEXT("Ground");
        Result.Label = Label;
        return Result;
    }
    
    if (Label.IsRobot())
    {
        FString RobotName = Label.Features.Contains(TEXT("name")) ? Label.Features[TEXT("name")] : TEXT("");
        if (!RobotName.IsEmpty())
        {
            AMACharacter* Robot = FindRobotByName(World, RobotName);
            if (Robot)
            {
                Result.Actor = Robot;
                Result.Name = Robot->AgentName;
                Result.Location = Robot->GetActorLocation();
                Result.Label = Label;
                Result.bFound = true;
                return Result;
            }
        }
    }
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAPickupItem::StaticClass(), FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        if (MatchesLabel(Actor, Label))
        {
            Result.Actor = Actor;
            Result.Name = Cast<AMAPickupItem>(Actor)->ItemName;
            Result.Location = Actor->GetActorLocation();
            Result.Label = ExtractLabel(Actor);
            Result.bFound = true;
            return Result;
        }
    }
    
    return Result;
}

TArray<FMASceneQueryResult> FMASceneQuery::FindObjectsInBoundary(UWorld* World, const FMASemanticLabel& Label, const TArray<FVector>& BoundaryVertices)
{
    TArray<FMASceneQueryResult> Results;
    if (!World || BoundaryVertices.Num() < 3) return Results;
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAPickupItem::StaticClass(), FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        if (!Label.IsEmpty() && !MatchesLabel(Actor, Label)) continue;
        
        FVector Location = Actor->GetActorLocation();
        if (FMAGeometryUtils::IsPointInPolygon(Location, BoundaryVertices))
        {
            FMASceneQueryResult Result;
            Result.Actor = Actor;
            Result.Name = Cast<AMAPickupItem>(Actor)->ItemName;
            Result.Location = Location;
            Result.Label = ExtractLabel(Actor);
            Result.bFound = true;
            Results.Add(Result);
        }
    }
    
    return Results;
}

FMASceneQueryResult FMASceneQuery::FindNearestObject(UWorld* World, const FMASemanticLabel& Label, const FVector& FromLocation)
{
    FMASceneQueryResult Result;
    if (!World) return Result;
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMAPickupItem::StaticClass(), FoundActors);
    
    float MinDistance = FLT_MAX;
    
    for (AActor* Actor : FoundActors)
    {
        if (!Label.IsEmpty() && !MatchesLabel(Actor, Label)) continue;
        
        float Distance = FVector::Dist(FromLocation, Actor->GetActorLocation());
        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            Result.Actor = Actor;
            Result.Name = Cast<AMAPickupItem>(Actor)->ItemName;
            Result.Location = Actor->GetActorLocation();
            Result.Label = ExtractLabel(Actor);
            Result.bFound = true;
        }
    }
    
    return Result;
}

bool FMASceneQuery::MatchesLabel(AActor* Actor, const FMASemanticLabel& Label)
{
    if (!Actor) return false;
    
    AMAPickupItem* Item = Cast<AMAPickupItem>(Actor);
    if (!Item) return false;
    
    FMASemanticLabel ActorLabel = ExtractLabel(Actor);
    
    if (!Label.Type.IsEmpty() && !ActorLabel.Type.Equals(Label.Type, ESearchCase::IgnoreCase))
    {
        return false;
    }
    
    for (const auto& Pair : Label.Features)
    {
        if (ActorLabel.Features.Contains(Pair.Key))
        {
            if (!ActorLabel.Features[Pair.Key].Equals(Pair.Value, ESearchCase::IgnoreCase))
            {
                return false;
            }
        }
    }
    
    return true;
}

FMASemanticLabel FMASceneQuery::ExtractLabel(AActor* Actor)
{
    FMASemanticLabel Label;
    if (!Actor) return Label;
    
    if (AMAPickupItem* Item = Cast<AMAPickupItem>(Actor))
    {
        Label.Class = TEXT("object");
        
        FString Name = Item->ItemName;
        TArray<FString> Parts;
        Name.ParseIntoArray(Parts, TEXT("_"));
        
        if (Parts.Num() >= 2)
        {
            Label.Features.Add(TEXT("color"), Parts[0]);
            Label.Type = Parts[1];
        }
        else if (Parts.Num() == 1)
        {
            Label.Type = Parts[0];
        }
        
        for (const FName& Tag : Actor->Tags)
        {
            FString TagStr = Tag.ToString();
            int32 ColonIndex;
            if (TagStr.FindChar(':', ColonIndex))
            {
                FString Key = TagStr.Left(ColonIndex);
                FString Value = TagStr.Mid(ColonIndex + 1);
                Label.Features.Add(Key, Value);
            }
        }
    }
    
    if (AMACharacter* Character = Cast<AMACharacter>(Actor))
    {
        Label.Class = TEXT("robot");
        Label.Type = TEXT("UGV");
        Label.Features.Add(TEXT("name"), Character->AgentName);
    }
    
    return Label;
}

AMACharacter* FMASceneQuery::FindRobotByName(UWorld* World, const FString& RobotName)
{
    if (!World || RobotName.IsEmpty()) return nullptr;
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, AMACharacter::StaticClass(), FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        AMACharacter* Character = Cast<AMACharacter>(Actor);
        if (Character && Character->AgentName.Equals(RobotName, ESearchCase::IgnoreCase))
        {
            return Character;
        }
    }
    
    return nullptr;
}
