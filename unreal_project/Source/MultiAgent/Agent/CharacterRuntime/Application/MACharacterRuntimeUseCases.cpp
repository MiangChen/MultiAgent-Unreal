#include "Agent/CharacterRuntime/Application/MACharacterRuntimeUseCases.h"

FMACharacterRuntimeFeedback FMACharacterRuntimeUseCases::BuildLowEnergyReturnPlan(const bool bIsFlying)
{
    FMACharacterRuntimeFeedback Feedback;
    Feedback.bSuccess = true;

    const bool bUseReturnHome = bIsFlying;
    Feedback.ReturnMode = bUseReturnHome ? EMACharacterReturnMode::ReturnHome : EMACharacterReturnMode::NavigateHome;
    Feedback.Severity = EMACharacterRuntimeSeverity::Warning;
    Feedback.Message = bUseReturnHome
        ? TEXT("Low battery, starting ReturnHome")
        : TEXT("Low battery, navigating back to initial position");
    return Feedback;
}
