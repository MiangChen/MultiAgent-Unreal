#include "MANotificationCoordinator.h"

#include "../Core/MAUITheme.h"

void FMANotificationCoordinator::BeginShow(FMANotificationAnimationState& State, EMANotificationType Type) const
{
    if (Type == EMANotificationType::None)
    {
        BeginHide(State);
        return;
    }

    if (State.bVisible && State.Type == Type && !State.bAnimating && State.bShowing)
    {
        return;
    }

    if (!State.bAnimating || State.bShowing)
    {
        State.CurrentOffsetX = -300.0f;
        State.CurrentOpacity = 0.0f;
    }

    State.Type = Type;
    State.bVisible = true;
    State.bShowing = true;
    State.bAnimating = true;
    State.CurrentAnimTime = 0.0f;
}

void FMANotificationCoordinator::BeginHide(FMANotificationAnimationState& State) const
{
    if (!State.bVisible)
    {
        return;
    }

    State.bShowing = false;
    State.bAnimating = true;
    State.CurrentAnimTime = 0.0f;
}

FMANotificationContentModel FMANotificationCoordinator::BuildContentModel(EMANotificationType Type, const UMAUITheme* Theme) const
{
    FMANotificationContentModel Model;
    Model.Message = GetNotificationMessage(Type);
    Model.KeyHint = GetKeyHint(Type);
    Model.BackgroundColor = GetBackgroundColorForType(Type, Theme);
    return Model;
}

FMANotificationAnimationFrame FMANotificationCoordinator::StepAnimation(FMANotificationAnimationState& State, float DeltaTime, float SlideInDuration, float SlideOutDuration, float StartOffsetX, float EndOffsetX) const
{
    FMANotificationAnimationFrame Frame;
    State.CurrentAnimTime += DeltaTime;

    const float Duration = State.bShowing ? SlideInDuration : SlideOutDuration;
    const float Progress = FMath::Clamp(State.CurrentAnimTime / Duration, 0.0f, 1.0f);
    const float Eased = 1.0f - FMath::Pow(1.0f - Progress, 3.0f);

    if (State.bShowing)
    {
        Frame.OffsetX = FMath::Lerp(StartOffsetX, EndOffsetX, Eased);
        Frame.Opacity = Eased;
    }
    else
    {
        Frame.OffsetX = FMath::Lerp(EndOffsetX, StartOffsetX, Eased);
        Frame.Opacity = 1.0f - Eased;
    }

    State.CurrentOffsetX = Frame.OffsetX;
    State.CurrentOpacity = Frame.Opacity;
    Frame.bFinished = Progress >= 1.0f;
    return Frame;
}

void FMANotificationCoordinator::FinishAnimation(FMANotificationAnimationState& State) const
{
    State.bAnimating = false;
    if (!State.bShowing)
    {
        State.bVisible = false;
        State.Type = EMANotificationType::None;
    }
}

FString FMANotificationCoordinator::GetNotificationMessage(EMANotificationType Type) const
{
    switch (Type)
    {
    case EMANotificationType::TaskGraphUpdate: return TEXT("Task Graph Updated");
    case EMANotificationType::SkillListUpdate: return TEXT("Skill List Updated");
    case EMANotificationType::SkillListExecuting: return TEXT("Skills Executing");
    case EMANotificationType::RequestUserCommand: return TEXT("Command Input Required");
    case EMANotificationType::SkillListPaused: return TEXT("Skill Execution Paused");
    case EMANotificationType::SkillListResumed: return TEXT("Skill Execution Resumed");
    case EMANotificationType::DecisionUpdate: return TEXT("Decision Required");
    default: return TEXT("");
    }
}

FString FMANotificationCoordinator::GetKeyHint(EMANotificationType Type) const
{
    switch (Type)
    {
    case EMANotificationType::TaskGraphUpdate: return TEXT("Press 'Z' to review Task Graph");
    case EMANotificationType::SkillListUpdate: return TEXT("Press 'N' to review Skill List");
    case EMANotificationType::SkillListExecuting: return TEXT("Skills are currently being executed");
    case EMANotificationType::RequestUserCommand: return TEXT("Please enter command in the right sidebar");
    case EMANotificationType::SkillListPaused: return TEXT("Press 'P' to resume execution");
    case EMANotificationType::SkillListResumed: return TEXT("Skills are continuing to execute");
    case EMANotificationType::DecisionUpdate: return TEXT("Press '9' to review Decision");
    default: return TEXT("");
    }
}

FLinearColor FMANotificationCoordinator::GetBackgroundColorForType(EMANotificationType Type, const UMAUITheme* Theme) const
{
    switch (Type)
    {
    case EMANotificationType::TaskGraphUpdate:
        if (Theme) { FLinearColor C = Theme->PrimaryColor; C.A = 0.95f; return C * 0.3f + FLinearColor(0.1f,0.1f,0.12f,0.95f) * 0.7f; }
        return FLinearColor(0.12f, 0.15f, 0.22f, 0.95f);
    case EMANotificationType::SkillListUpdate:
        if (Theme) { FLinearColor C = Theme->SuccessColor; C.A = 0.95f; return C * 0.3f + FLinearColor(0.1f,0.1f,0.12f,0.95f) * 0.7f; }
        return FLinearColor(0.12f, 0.18f, 0.14f, 0.95f);
    case EMANotificationType::SkillListExecuting:
        if (Theme) { FLinearColor C = Theme->WarningColor; C.A = 0.95f; return C * 0.3f + FLinearColor(0.1f,0.1f,0.12f,0.95f) * 0.7f; }
        return FLinearColor(0.22f, 0.20f, 0.12f, 0.95f);
    case EMANotificationType::RequestUserCommand:
        return FLinearColor(0.1f, 0.3f, 0.6f, 0.9f);
    case EMANotificationType::SkillListPaused:
        if (Theme) { FLinearColor C = Theme->WarningColor; C.A = 0.95f; return C * 0.4f + FLinearColor(0.1f,0.1f,0.12f,0.95f) * 0.6f; }
        return FLinearColor(0.25f, 0.18f, 0.08f, 0.95f);
    case EMANotificationType::SkillListResumed:
        if (Theme) { FLinearColor C = Theme->SuccessColor; C.A = 0.95f; return C * 0.3f + FLinearColor(0.1f,0.1f,0.12f,0.95f) * 0.7f; }
        return FLinearColor(0.12f, 0.20f, 0.14f, 0.95f);
    case EMANotificationType::DecisionUpdate:
        if (Theme) { FLinearColor C = Theme->WarningColor; C.A = 0.95f; return C * 0.35f + FLinearColor(0.1f,0.1f,0.12f,0.95f) * 0.65f; }
        return FLinearColor(0.24f, 0.19f, 0.10f, 0.95f);
    default:
        return Theme ? Theme->BackgroundColor : FLinearColor(0.1f, 0.1f, 0.12f, 0.95f);
    }
}
