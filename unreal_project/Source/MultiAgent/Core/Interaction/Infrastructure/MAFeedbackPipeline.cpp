// Translate staged control feedback from L5 observations to L1 UI feedback.

#include "MAFeedbackPipeline.h"

FMAFeedback43 FMAFeedbackPipeline::ToFeedback43(const FMAFeedback54& Feedback54) const
{
    FMAFeedback43 Feedback43;
    Feedback43.bSuccess = Feedback54.bSuccess;
    Feedback43.Count = Feedback54.Count;
    Feedback43.SubjectId = Feedback54.SubjectId;
    Feedback43.Message = Feedback54.Message;
    Feedback43.Event = Feedback54.bSuccess
        ? EMAFeedback43Event::ExecutionAccepted
        : EMAFeedback43Event::ExecutionRejected;
    Feedback43.bIsWarning = !Feedback54.bSuccess;
    return Feedback43;
}

FMAFeedback32 FMAFeedbackPipeline::ToFeedback32(const FMAFeedback43& Feedback43) const
{
    FMAFeedback32 Feedback32;
    Feedback32.bSuccess = Feedback43.bSuccess;
    Feedback32.Count = Feedback43.Count;
    Feedback32.SubjectId = Feedback43.SubjectId;
    Feedback32.Message = Feedback43.Message;
    Feedback32.Event = Feedback43.bSuccess
        ? EMAFeedback32Event::SceneActionPrepared
        : EMAFeedback32Event::ModeTransitionRejected;
    return Feedback32;
}

FMAFeedback21Batch FMAFeedbackPipeline::ToFeedback21(const FMAFeedback32& Feedback32) const
{
    FMAFeedback21Batch Feedback21;
    if (Feedback32.Message.IsEmpty())
    {
        return Feedback21;
    }

    Feedback21.AddMessage(
        Feedback32.Message,
        Feedback32.bSuccess ? EMAFeedback21MessageSeverity::Success : EMAFeedback21MessageSeverity::Warning,
        3.f);
    return Feedback21;
}
