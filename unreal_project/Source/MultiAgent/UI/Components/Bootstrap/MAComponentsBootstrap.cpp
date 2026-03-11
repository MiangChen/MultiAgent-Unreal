#include "MAComponentsBootstrap.h"

#include "UI/Components/MADirectControlIndicator.h"
#include "UI/Components/MASystemLogPanel.h"
#include "UI/Components/MAPreviewPanel.h"
#include "UI/Components/MAInstructionPanel.h"
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
