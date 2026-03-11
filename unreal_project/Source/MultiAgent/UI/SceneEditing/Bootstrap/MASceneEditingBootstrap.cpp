#include "MASceneEditingBootstrap.h"

#include "UI/SceneEditing/MAModifyWidget.h"
#include "UI/SceneEditing/MAEditWidget.h"
#include "UI/SceneEditing/MASceneListWidget.h"
#include "UI/Core/MAUITheme.h"

void FMASceneEditingBootstrap::ConfigureModifyWidget(UMAModifyWidget* Widget, UMAUITheme* Theme) const
{
    if (Widget && Theme)
    {
        Widget->ApplyTheme(Theme);
    }
}

void FMASceneEditingBootstrap::ConfigureEditWidget(UMAEditWidget* Widget, UMAUITheme* Theme) const
{
    if (Widget && Theme)
    {
        Widget->ApplyTheme(Theme);
    }
}

void FMASceneEditingBootstrap::ConfigureSceneListWidget(UMASceneListWidget* Widget, UMAUITheme* Theme) const
{
    if (Widget && Theme)
    {
        Widget->ApplyTheme(Theme);
    }
}
