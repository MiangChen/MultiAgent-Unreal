#include "MASkillAllocationUIBootstrap.h"

#include "UI/SkillAllocation/MASkillAllocationViewer.h"
#include "UI/SkillAllocation/MAGanttCanvas.h"
#include "UI/Core/MAUITheme.h"

void FMASkillAllocationUIBootstrap::ConfigureSkillAllocationViewer(UMASkillAllocationViewer* Viewer, UMAUITheme* Theme) const
{
    if (!Viewer)
    {
        return;
    }

    if (Theme)
    {
        if (UMAGanttCanvas* GanttCanvas = Viewer->GetGanttCanvas())
        {
            GanttCanvas->ApplyTheme(Theme);
        }
    }

    Viewer->LoadMockData();
}
