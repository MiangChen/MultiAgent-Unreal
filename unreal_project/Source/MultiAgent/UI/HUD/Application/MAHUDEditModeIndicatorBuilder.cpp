// Builder for edit-mode indicator list content.

#include "MAHUDEditModeIndicatorBuilder.h"
#include "../Infrastructure/MAHUDEditRuntimeAdapter.h"

bool FMAHUDEditModeIndicatorBuilder::Build(AMAHUD* HUD, FMAHUDEditModeIndicatorModel& OutModel) const
{
    return RuntimeAdapter.BuildEditModeIndicatorModel(HUD, OutModel);
}
