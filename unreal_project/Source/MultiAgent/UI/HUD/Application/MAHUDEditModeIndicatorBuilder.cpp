// Builder for edit-mode indicator list content.

#include "MAHUDEditModeIndicatorBuilder.h"
#include "../Runtime/MAHUD.h"

bool FMAHUDEditModeIndicatorBuilder::Build(AMAHUD* HUD, FMAHUDEditModeIndicatorModel& OutModel) const
{
    return HUD && HUD->RuntimeBuildEditModeIndicatorModel(OutModel);
}
