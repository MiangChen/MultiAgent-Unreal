#pragma once

#include "CoreMinimal.h"
#include "MASkillAllocationTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMASkillAllocation, Log, All);

UENUM(BlueprintType)
enum class ESkillExecutionStatus : uint8
{
    Pending     UMETA(DisplayName = "Pending"),
    InProgress  UMETA(DisplayName = "In Progress"),
    Completed   UMETA(DisplayName = "Completed"),
    Failed      UMETA(DisplayName = "Failed")
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillAssignment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString ParamsJson;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    ESkillExecutionStatus Status = ESkillExecutionStatus::Pending;

    FMASkillAssignment() {}

    FMASkillAssignment(const FString& InSkillName, const FString& InParamsJson)
        : SkillName(InSkillName), ParamsJson(InParamsJson) {}

    bool operator==(const FMASkillAssignment& Other) const;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMATimeStepData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    TMap<FString, FMASkillAssignment> RobotSkills;

    FMATimeStepData() {}

    bool operator==(const FMATimeStepData& Other) const;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillAllocationData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    TMap<int32, FMATimeStepData> Data;

    FMASkillAllocationData() {}

    TArray<FString> GetAllRobotIds() const;
    TArray<int32> GetAllTimeSteps() const;
    FMASkillAssignment* FindSkill(int32 TimeStep, const FString& RobotId);
    const FMASkillAssignment* FindSkill(int32 TimeStep, const FString& RobotId) const;
    bool ValidateRobotIds(TArray<FString>& OutUndefinedRobots) const;
    bool operator==(const FMASkillAllocationData& Other) const;
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FMASkillBarRenderData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    int32 TimeStep = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString RobotId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FString ParamsJson;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    ESkillExecutionStatus Status = ESkillExecutionStatus::Pending;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FVector2D Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FVector2D Size = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillAllocation")
    bool bIsSelected = false;

    FMASkillBarRenderData() {}

    FMASkillBarRenderData(int32 InTimeStep, const FString& InRobotId, const FMASkillAssignment& InSkill)
        : TimeStep(InTimeStep), RobotId(InRobotId), SkillName(InSkill.SkillName), ParamsJson(InSkill.ParamsJson), Status(InSkill.Status) {}
};

UENUM(BlueprintType)
enum class EGanttDragState : uint8
{
    Idle        UMETA(DisplayName = "Idle"),
    Potential   UMETA(DisplayName = "Potential"),
    Dragging    UMETA(DisplayName = "Dragging")
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FGanttDragSource
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    int32 TimeStep = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString RobotId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString ParamsJson;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    ESkillExecutionStatus Status = ESkillExecutionStatus::Pending;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FVector2D OriginalPosition = FVector2D::ZeroVector;

    FGanttDragSource() {}

    FGanttDragSource(int32 InTimeStep, const FString& InRobotId, const FString& InSkillName,
                     const FString& InParamsJson, ESkillExecutionStatus InStatus, const FVector2D& InPosition)
        : TimeStep(InTimeStep), RobotId(InRobotId), SkillName(InSkillName), ParamsJson(InParamsJson), Status(InStatus), OriginalPosition(InPosition) {}

    bool IsValid() const { return TimeStep >= 0 && !RobotId.IsEmpty(); }

    void Reset()
    {
        TimeStep = -1;
        RobotId.Empty();
        SkillName.Empty();
        ParamsJson.Empty();
        Status = ESkillExecutionStatus::Pending;
        OriginalPosition = FVector2D::ZeroVector;
    }
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FGanttDropTarget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    int32 TimeStep = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString RobotId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    bool bIsValid = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    bool bIsSnapped = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FVector2D SnapPosition = FVector2D::ZeroVector;

    FGanttDropTarget() {}

    FGanttDropTarget(int32 InTimeStep, const FString& InRobotId, bool bInIsValid, bool bInIsSnapped, const FVector2D& InSnapPosition)
        : TimeStep(InTimeStep), RobotId(InRobotId), bIsValid(bInIsValid), bIsSnapped(bInIsSnapped), SnapPosition(InSnapPosition) {}

    bool HasTarget() const { return TimeStep >= 0 && !RobotId.IsEmpty(); }

    void Reset()
    {
        TimeStep = -1;
        RobotId.Empty();
        bIsValid = false;
        bIsSnapped = false;
        SnapPosition = FVector2D::ZeroVector;
    }
};

USTRUCT(BlueprintType)
struct MULTIAGENT_API FGanttDragPreview
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FVector2D Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FVector2D Size = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 0.7f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GanttDrag")
    FString SkillName;

    FGanttDragPreview() {}

    FGanttDragPreview(const FVector2D& InPosition, const FVector2D& InSize, const FLinearColor& InColor, const FString& InSkillName)
        : Position(InPosition), Size(InSize), Color(InColor), SkillName(InSkillName) {}

    void Reset()
    {
        Position = FVector2D::ZeroVector;
        Size = FVector2D::ZeroVector;
        Color = FLinearColor(1.0f, 1.0f, 1.0f, 0.7f);
        SkillName.Empty();
    }
};
