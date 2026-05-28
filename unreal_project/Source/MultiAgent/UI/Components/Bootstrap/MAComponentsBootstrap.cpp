#include "MAComponentsBootstrap.h"

#include "UI/Components/Presentation/MADirectControlIndicator.h"
#include "UI/Components/Presentation/MASystemLogPanel.h"
#include "UI/Components/Presentation/MAPreviewPanel.h"
#include "UI/Components/Presentation/MAInstructionPanel.h"
#include "UI/Core/MAUITheme.h"

void FMAComponentsBootstrap::ConfigureDirectControlIndicator(UMADirectControlIndicator* Widget, UMAUITheme* Theme) const
{
    if (Widget && Theme)
    {
        Widget->ApplyTheme(Theme);
    }
}

void FMAComponentsBootstrap::ConfigureSystemLogPanel(UMASystemLogPanel* Widget, UMAUITheme* Theme) const
{
    if (Widget && Theme)
    {
        Widget->ApplyTheme(Theme);
    }
}

void FMAComponentsBootstrap::ConfigurePreviewPanel(UMAPreviewPanel* Widget, UMAUITheme* Theme) const
{
    if (Widget && Theme)
    {
        Widget->ApplyTheme(Theme);
    }
}

void FMAComponentsBootstrap::ConfigureInstructionPanel(UMAInstructionPanel* Widget, UMAUITheme* Theme) const
{
    if (Widget && Theme)
    {
        Widget->ApplyTheme(Theme);
    }
}
