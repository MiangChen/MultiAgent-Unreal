#pragma once

#include "MAInputTypes.h"

struct FMAMouseModeState
{
    EMAMouseMode CurrentMode = EMAMouseMode::Select;
    EMAMouseMode PreviousMode = EMAMouseMode::Select;

    bool Is(EMAMouseMode Mode) const
    {
        return CurrentMode == Mode;
    }
};
