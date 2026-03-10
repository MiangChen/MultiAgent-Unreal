#pragma once

#include "CoreMinimal.h"
#include "MATaskGraphTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMATaskGraph, Log, All);

class UTexture2D;

UENUM(BlueprintType)
enum class EMATaskExecutionStatus : uint8
{
    Pending     UMETA(DisplayName = "Pending"),
    InProgress  UMETA(DisplayName = "In Progress"),
    Completed   UMETA(DisplayName = "Completed"),
    Failed      UMETA(DisplayName = "Failed")
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMARequiredSkill
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FString> AssignedRobotType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    int32 AssignedRobotCount = 1;

    FMARequiredSkill() {}

    FMARequiredSkill(const FString& InSkillName, const TArray<FString>& InRobotTypes, int32 InCount = 1)
        : SkillName(InSkillName), AssignedRobotType(InRobotTypes), AssignedRobotCount(InCount) {}

    bool operator==(const FMARequiredSkill& Other) const;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskNodeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString TaskId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString Location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FMARequiredSkill> RequiredSkills;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FString> Produces;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FVector2D CanvasPosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    EMATaskExecutionStatus Status = EMATaskExecutionStatus::Pending;

    FMATaskNodeData() {}

    FMATaskNodeData(const FString& InTaskId, const FString& InDescription, const FString& InLocation)
        : TaskId(InTaskId), Description(InDescription), Location(InLocation) {}

    bool operator==(const FMATaskNodeData& Other) const;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskEdgeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString FromNodeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString ToNodeId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString EdgeType = TEXT("sequential");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString Condition;

    FMATaskEdgeData() {}

    FMATaskEdgeData(const FString& InFrom, const FString& InTo, const FString& InType = TEXT("sequential"))
        : FromNodeId(InFrom), ToNodeId(InTo), EdgeType(InType) {}

    bool operator==(const FMATaskEdgeData& Other) const;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATaskGraphData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FMATaskNodeData> Nodes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    TArray<FMATaskEdgeData> Edges;

    FMATaskGraphData() {}

    FMATaskNodeData* FindNode(const FString& TaskId);
    const FMATaskNodeData* FindNode(const FString& TaskId) const;
    bool HasNode(const FString& TaskId) const;
    bool HasEdge(const FString& FromId, const FString& ToId) const;
    bool IsValidDAG() const;
    bool operator==(const FMATaskGraphData& Other) const;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMANodeTemplate
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString TemplateName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString DefaultDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    FString DefaultLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TaskGraph")
    UTexture2D* Icon = nullptr;

    FMANodeTemplate() {}

    FMANodeTemplate(const FString& InName, const FString& InDesc, const FString& InLocation)
        : TemplateName(InName), DefaultDescription(InDesc), DefaultLocation(InLocation) {}
};
