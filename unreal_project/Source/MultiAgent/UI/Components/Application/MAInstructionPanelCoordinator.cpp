#include "MAInstructionPanelCoordinator.h"

#include "../Infrastructure/MAInstructionPanelRuntimeAdapter.h"
#include "Blueprint/UserWidget.h"

FMAInstructionPanelActionResult FMAInstructionPanelCoordinator::BuildSubmitResult(const FString& Command) const
{
    FMAInstructionPanelActionResult Result;
    Result.Command = Command.TrimStartAndEnd();
    Result.bAccepted = !Result.Command.IsEmpty();
    Result.bDismissRequestNotification = Result.bAccepted;
    return Result;
}

void FMAInstructionPanelCoordinator::ApplyPostSubmit(const UUserWidget* Context, const FMAInstructionPanelActionResult& Result) const
{
    if (!Result.bDismissRequestNotification)
    {
        return;
    }

    const FMAInstructionPanelRuntimeAdapter RuntimeAdapter;
    RuntimeAdapter.DismissRequestNotification(Context);
}
