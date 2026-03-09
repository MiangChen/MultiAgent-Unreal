#pragma once

#include "CoreMinimal.h"
#include "../MAModifyTypes.h"
#include "../Infrastructure/MAModifyWidgetInputParser.h"
#include "../Infrastructure/MAModifyWidgetNodeBuilder.h"
#include "../Infrastructure/MAModifyWidgetSceneGraphAdapter.h"

class AActor;
class UWorld;

enum class EMAModifySubmissionKind : uint8
{
    None,
    Single,
    Multi
};

struct FMAModifyWidgetSubmission
{
    EMAModifySubmissionKind Kind = EMAModifySubmissionKind::None;
    AActor* PrimaryActor = nullptr;
    TArray<AActor*> Actors;
    FString LabelText;
    FString GeneratedJson;
};

class MULTIAGENT_API FMAModifyWidgetCoordinator
{
public:
    bool PrepareSubmission(
        UWorld* World,
        const TArray<AActor*>& SelectedActors,
        EMAAnnotationMode AnnotationMode,
        const FString& EditingNodeId,
        const FString& LabelContent,
        FMAModifyWidgetSubmission& OutSubmission) const;

private:
    FMAModifyWidgetInputParser InputParser;
    FMAModifyWidgetNodeBuilder NodeBuilder;
    FMAModifyWidgetSceneGraphAdapter SceneGraphAdapter;
};
