#pragma once

#include "CoreMinimal.h"
#include "MACommTaskPlanTypes.generated.h"

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskPlanNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    FString NodeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    FString TaskType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    TMap<FString, FString> Parameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    TArray<FString> Dependencies;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskPlanEdge
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    FString FromNodeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    FString ToNodeId;

    FMATaskPlanEdge() {}

    FMATaskPlanEdge(const FString& InFrom, const FString& InTo)
        : FromNodeId(InFrom), ToNodeId(InTo) {}
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskPlan
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    TArray<FMATaskPlanNode> Nodes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskPlan")
    TArray<FMATaskPlanEdge> Edges;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAWorldModelEntity
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString EntityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString EntityType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    TMap<FString, FString> Properties;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAWorldModelRelationship
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString EntityAId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString EntityBId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    FString RelationshipType;

    FMAWorldModelRelationship() {}

    FMAWorldModelRelationship(const FString& InEntityA, const FString& InEntityB, const FString& InType)
        : EntityAId(InEntityA), EntityBId(InEntityB), RelationshipType(InType) {}
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMAWorldModelGraph
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    TArray<FMAWorldModelEntity> Entities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldModel")
    TArray<FMAWorldModelRelationship> Relationships;
};
