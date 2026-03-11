#include "MASkillParamsProcessor.h"
#include "MASkillParamsProcessorInternal.h"

#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace MAParamsHelper
{
bool ParseRawParams(const FString& RawParamsJson, TSharedPtr<FJsonObject>& OutJson)
{
    if (RawParamsJson.IsEmpty())
    {
        return false;
    }
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawParamsJson);
    return FJsonSerializer::Deserialize(Reader, OutJson) && OutJson.IsValid();
}

bool ExtractDestPosition(const TSharedPtr<FJsonObject>& Json, FVector& OutPosition)
{
    const TSharedPtr<FJsonObject>* DestObject;
    if (Json->TryGetObjectField(TEXT("dest"), DestObject))
    {
        double X = 0, Y = 0, Z = 0;
        (*DestObject)->TryGetNumberField(TEXT("x"), X);
        (*DestObject)->TryGetNumberField(TEXT("y"), Y);
        (*DestObject)->TryGetNumberField(TEXT("z"), Z);
        OutPosition = FVector(X, Y, Z);
        return true;
    }
    return false;
}

bool ExtractSearchArea(const TSharedPtr<FJsonObject>& Json, TArray<FVector>& OutBoundary)
{
    OutBoundary.Empty();

    const TSharedPtr<FJsonObject>* AreaObject;
    if (Json->TryGetObjectField(TEXT("area"), AreaObject))
    {
        const TArray<TSharedPtr<FJsonValue>>* CoordsArray;
        if ((*AreaObject)->TryGetArrayField(TEXT("coords"), CoordsArray))
        {
            for (const auto& CoordValue : *CoordsArray)
            {
                const TArray<TSharedPtr<FJsonValue>>* PointArray;
                if (CoordValue->TryGetArray(PointArray) && PointArray->Num() >= 2)
                {
                    const double PX = (*PointArray)[0]->AsNumber();
                    const double PY = (*PointArray)[1]->AsNumber();
                    OutBoundary.Add(FVector(PX, PY, 0.0f));
                }
            }
        }
    }

    if (OutBoundary.Num() == 0)
    {
        const TArray<TSharedPtr<FJsonValue>>* SearchAreaArray;
        if (Json->TryGetArrayField(TEXT("search_area"), SearchAreaArray))
        {
            for (const auto& CoordValue : *SearchAreaArray)
            {
                const TArray<TSharedPtr<FJsonValue>>* PointArray;
                if (CoordValue->TryGetArray(PointArray) && PointArray->Num() >= 2)
                {
                    const double PX = (*PointArray)[0]->AsNumber();
                    const double PY = (*PointArray)[1]->AsNumber();
                    OutBoundary.Add(FVector(PX, PY, 0.0f));
                }
            }
        }
    }

    return OutBoundary.Num() >= 3;
}

bool ExtractTargetJson(const TSharedPtr<FJsonObject>& Json, const FString& FieldName, FString& OutTargetJson)
{
    const TSharedPtr<FJsonObject>* TargetObj;
    if (Json->TryGetObjectField(FieldName, TargetObj))
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutTargetJson);
        FJsonSerializer::Serialize((*TargetObj).ToSharedRef(), Writer);
        return true;
    }
    return false;
}

FString ExtractTaskId(const TSharedPtr<FJsonObject>& Json)
{
    FString TaskId;
    Json->TryGetStringField(TEXT("task_id"), TaskId);
    return TaskId;
}

FString ExtractObjectId(const TSharedPtr<FJsonObject>& Json)
{
    FString ObjectId;
    Json->TryGetStringField(TEXT("object_id"), ObjectId);
    return ObjectId;
}
}

namespace MASkillParamsProcessorInternal
{
float ComputeHorizontalDistanceMeters(const AMACharacter* Agent, const FMAResolvedSkillTarget& Target)
{
    if (!Agent || !Target.bFound)
    {
        return -1.f;
    }

    const FVector TargetLocation = Target.Actor.IsValid() ? Target.Actor->GetActorLocation() : Target.Location;
    return FVector::Dist2D(Agent->GetActorLocation(), TargetLocation) / 100.f;
}
}

void FMASkillParamsProcessor::Process(AMACharacter* Agent, EMACommand Command, const FMAAgentSkillCommand* Cmd)
{
    if (!Agent) return;

    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    if (!SkillComp) return;

    SkillComp->ResetFeedbackContext();
    SkillComp->ResetSkillRuntimeTargets();
    SkillComp->ResetSearchResults();

    if (Cmd && !Cmd->Params.RawParamsJson.IsEmpty())
    {
        TSharedPtr<FJsonObject> ParamsJson;
        if (MAParamsHelper::ParseRawParams(Cmd->Params.RawParamsJson, ParamsJson))
        {
            const FString TaskId = MAParamsHelper::ExtractTaskId(ParamsJson);
            if (!TaskId.IsEmpty())
            {
                SkillComp->GetFeedbackContextMutable().TaskId = TaskId;
            }
        }
    }

    switch (Command)
    {
        case EMACommand::Navigate: ProcessNavigate(SkillComp, Cmd); break;
        case EMACommand::Search: ProcessSearch(Agent, SkillComp, Cmd); break;
        case EMACommand::Follow: ProcessFollow(SkillComp, Cmd); break;
        case EMACommand::Charge: ProcessCharge(SkillComp, Cmd); break;
        case EMACommand::Place: ProcessPlace(Agent, SkillComp, Cmd); break;
        case EMACommand::TakeOff: ProcessTakeOff(SkillComp, Cmd); break;
        case EMACommand::Land: ProcessLand(SkillComp, Cmd); break;
        case EMACommand::ReturnHome: ProcessReturnHome(SkillComp, Cmd); break;
        case EMACommand::TakePhoto: ProcessTakePhoto(Agent, SkillComp, Cmd); break;
        case EMACommand::Broadcast: ProcessBroadcast(Agent, SkillComp, Cmd); break;
        case EMACommand::HandleHazard: ProcessHandleHazard(Agent, SkillComp, Cmd); break;
        case EMACommand::Guide: ProcessGuide(Agent, SkillComp, Cmd); break;
        default: break;
    }
}
