// Builder for edit-mode indicator list content.

#include "MAHUDEditModeIndicatorBuilder.h"
#include "../MAHUD.h"

bool FMAHUDEditModeIndicatorBuilder::Build(AMAHUD* HUD, FMAHUDEditModeIndicatorModel& OutModel) const
{
    return HUD && HUD->RuntimeBuildEditModeIndicatorModel(OutModel);
}
