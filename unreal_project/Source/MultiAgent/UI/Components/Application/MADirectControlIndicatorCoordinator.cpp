#include "MADirectControlIndicatorCoordinator.h"

#include "../Core/MAUITheme.h"

FMADirectControlIndicatorModel FMADirectControlIndicatorCoordinator::BuildModel(const FString& AgentLabel, const UMAUITheme* Theme, const FLinearColor& DefaultColor) const
{
    FMADirectControlIndicatorModel Model;
    Model.DisplayText = FString::Printf(TEXT("Direct Control: %s"), AgentLabel.IsEmpty() ? TEXT("---") : *AgentLabel);
    Model.TextColor = Theme ? Theme->SuccessColor : DefaultColor;
    return Model;
}
