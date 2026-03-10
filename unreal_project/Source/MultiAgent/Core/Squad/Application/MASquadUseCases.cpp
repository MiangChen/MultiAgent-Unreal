#include "Core/Squad/Application/MASquadUseCases.h"

#include "Core/Squad/Domain/MASquad.h"

FMASquadOperationFeedback FMASquadUseCases::BuildRuntimeUnavailable(const EMASquadOperationKind Kind)
{
    FMASquadOperationFeedback Feedback;
    Feedback.Kind = Kind;
    Feedback.Severity = EMASquadFeedbackSeverity::Error;
    Feedback.Message = TEXT("Squad runtime unavailable");
    return Feedback;
}

FMASquadOperationFeedback FMASquadUseCases::BuildMissingDefaultSquad()
{
    FMASquadOperationFeedback Feedback;
    Feedback.Kind = EMASquadOperationKind::CycleFormation;
    Feedback.Severity = EMASquadFeedbackSeverity::Warning;
    Feedback.Message = TEXT("Need a humanoid leader and at least 2 agents to cycle formation");
    return Feedback;
}

FMASquadOperationFeedback FMASquadUseCases::BuildSelectionTooSmall(const int32 SelectedCount)
{
    FMASquadOperationFeedback Feedback;
    Feedback.Kind = EMASquadOperationKind::CreateSquad;
    Feedback.Severity = EMASquadFeedbackSeverity::Warning;
    Feedback.Message = SelectedCount <= 0
        ? TEXT("Select at least 2 agents to create squad (Q)")
        : FString::Printf(TEXT("Need %d more agent(s) to create squad"), 2 - SelectedCount);
    Feedback.AffectedCount = SelectedCount;
    return Feedback;
}

FMASquadOperationFeedback FMASquadUseCases::BuildCreateFailed()
{
    FMASquadOperationFeedback Feedback;
    Feedback.Kind = EMASquadOperationKind::CreateSquad;
    Feedback.Severity = EMASquadFeedbackSeverity::Warning;
    Feedback.Message = TEXT("Failed to create squad");
    return Feedback;
}

FMASquadOperationFeedback FMASquadUseCases::BuildCreated(const UMASquad& Squad)
{
    FMASquadOperationFeedback Feedback;
    Feedback.Kind = EMASquadOperationKind::CreateSquad;
    Feedback.Severity = EMASquadFeedbackSeverity::Success;
    Feedback.bSuccess = true;
    Feedback.SquadName = Squad.SquadName;
    Feedback.AffectedCount = Squad.GetMemberCount();
    Feedback.Message = FString::Printf(
        TEXT("Created Squad '%s' with %d members"),
        *Feedback.SquadName,
        Feedback.AffectedCount);
    return Feedback;
}

FMASquadOperationFeedback FMASquadUseCases::BuildDisbandEmpty()
{
    FMASquadOperationFeedback Feedback;
    Feedback.Kind = EMASquadOperationKind::DisbandSquad;
    Feedback.Severity = EMASquadFeedbackSeverity::Warning;
    Feedback.Message = TEXT("No squad to disband (Shift+Q)");
    return Feedback;
}

FMASquadOperationFeedback FMASquadUseCases::BuildDisbanded(const int32 DisbandedCount)
{
    FMASquadOperationFeedback Feedback;
    Feedback.Kind = EMASquadOperationKind::DisbandSquad;
    Feedback.Severity = EMASquadFeedbackSeverity::Warning;
    Feedback.bSuccess = DisbandedCount > 0;
    Feedback.AffectedCount = DisbandedCount;
    Feedback.Message = FString::Printf(TEXT("Disbanded %d squad(s)"), DisbandedCount);
    return Feedback;
}

FMASquadOperationFeedback FMASquadUseCases::BuildCycleResult(const UMASquad& Squad)
{
    FMASquadOperationFeedback Feedback;
    Feedback.Kind = EMASquadOperationKind::CycleFormation;
    Feedback.bSuccess = true;
    Feedback.SquadName = Squad.SquadName;
    Feedback.FormationType = Squad.GetFormationType();

    if (Feedback.FormationType == EMAFormationType::None)
    {
        Feedback.Severity = EMASquadFeedbackSeverity::Info;
        Feedback.Message = FString::Printf(TEXT("[%s] Formation stopped"), *Feedback.SquadName);
    }
    else
    {
        Feedback.Severity = EMASquadFeedbackSeverity::Success;
        Feedback.Message = FString::Printf(
            TEXT("[%s] Formation: %s"),
            *Feedback.SquadName,
            *UMASquad::FormationTypeToString(Feedback.FormationType));
    }

    return Feedback;
}
