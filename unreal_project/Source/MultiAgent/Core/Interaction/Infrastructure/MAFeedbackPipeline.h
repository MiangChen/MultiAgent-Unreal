#pragma once

#include "CoreMinimal.h"
#include "../Feedback/MAFeedback21.h"
#include "../Feedback/MAFeedback32.h"
#include "../Feedback/MAFeedback43.h"
#include "../Feedback/MAFeedback54.h"

class MULTIAGENT_API FMAFeedbackPipeline
{
public:
    FMAFeedback43 ToFeedback43(const FMAFeedback54& Feedback54) const;
    FMAFeedback32 ToFeedback32(const FMAFeedback43& Feedback43) const;
    FMAFeedback21Batch ToFeedback21(const FMAFeedback32& Feedback32) const;
};
