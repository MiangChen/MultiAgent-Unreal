#include "Agent/Navigation/Application/MANavigationUseCases.h"

#include "GameFramework/Character.h"

FMANavigationCommandFeedback FMANavigationUseCases::BuildFlightCommandPrecheck(
    const ACharacter* OwnerCharacter,
    const bool bIsFlying,
    const bool bHasFlightController,
    const TCHAR* CommandName)
{
    FMANavigationCommandFeedback Feedback;

    if (!OwnerCharacter)
    {
        Feedback.Severity = EMANavigationFeedbackSeverity::Error;
        Feedback.Message = FString::Printf(TEXT("%s failed: No owner character"), CommandName);
        return Feedback;
    }

    if (!bIsFlying)
    {
        Feedback.Severity = EMANavigationFeedbackSeverity::Warning;
        Feedback.Message = FString::Printf(TEXT("%s failed: Not a flying vehicle"), CommandName);
        return Feedback;
    }

    if (!bHasFlightController)
    {
        Feedback.Severity = EMANavigationFeedbackSeverity::Warning;
        Feedback.Message = FString::Printf(TEXT("%s failed: FlightController not initialized"), CommandName);
        return Feedback;
    }

    Feedback.bSuccess = true;
    Feedback.Severity = EMANavigationFeedbackSeverity::Info;
    Feedback.Message = FString::Printf(TEXT("%s precheck passed"), CommandName);
    return Feedback;
}
