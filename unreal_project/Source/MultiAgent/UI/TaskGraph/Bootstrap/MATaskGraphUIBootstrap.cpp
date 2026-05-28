#include "MATaskGraphUIBootstrap.h"

#include "UI/TaskGraph/Presentation/MATaskPlannerWidget.h"
#include "UI/TaskGraph/Presentation/MANodePaletteWidget.h"
#include "UI/Core/MAUITheme.h"
#include "Core/TaskGraph/Application/MATaskGraphUseCases.h"

void FMATaskGraphUIBootstrap::ConfigureTaskPlannerWidget(UMATaskPlannerWidget* Widget, UMAUITheme* Theme) const
{
    if (!Widget)
    {
        return;
    }

    if (Theme)
    {
        Widget->ApplyTheme(Theme);
    }

    if (UMANodePaletteWidget* NodePalette = Widget->GetNodePalette())
    {
        InitializeNodePalette(NodePalette);
    }

    if (Widget->IsMockMode())
    {
        Widget->LoadMockData();
    }
}

void FMATaskGraphUIBootstrap::InitializeNodePalette(UMANodePaletteWidget* NodePalette) const
{
    if (NodePalette)
    {
        NodePalette->InitializeTemplates(FTaskGraphUseCases::BuildDefaultNodeTemplates());
    }
}
