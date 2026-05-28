#include "MAModalCoordinator.h"

FMAModalButtonModel FMAModalCoordinator::BuildButtonModel(bool bShowEditButton, bool bIsEditMode) const
{
    FMAModalButtonModel Model;
    Model.EditVisibility = (bShowEditButton && !bIsEditMode) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    return Model;
}

FMAModalAnimationStep FMAModalCoordinator::StepAnimation(float CurrentTime, float DeltaTime, bool bIsShowing, float Duration) const
{
    FMAModalAnimationStep Step;
    Step.NextTime = CurrentTime + DeltaTime;

    const float Progress = FMath::Clamp(Step.NextTime / Duration, 0.0f, 1.0f);
    const float Eased = FMath::InterpEaseInOut(0.0f, 1.0f, Progress, 2.0f);
    Step.Opacity = bIsShowing ? Eased : (1.0f - Eased);
    Step.bFinished = Progress >= 1.0f;
    return Step;
}
