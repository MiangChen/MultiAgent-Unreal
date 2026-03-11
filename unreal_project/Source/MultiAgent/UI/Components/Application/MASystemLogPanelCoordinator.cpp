#include "MASystemLogPanelCoordinator.h"

#include "../Presentation/MASystemLogPanel.h"
#include "../Core/MAUITheme.h"

void FMASystemLogPanelCoordinator::AppendLog(TArray<FMALogEntry>& Entries, const FString& Message, bool bIsError, int32 MaxEntries) const
{
    Entries.Add(FMALogEntry(Message, bIsError));
    while (Entries.Num() > MaxEntries)
    {
        Entries.RemoveAt(0);
    }
}

void FMASystemLogPanelCoordinator::ClearLogs(TArray<FMALogEntry>& Entries) const
{
    Entries.Reset();
}

FMALogEntryViewModel FMASystemLogPanelCoordinator::BuildEntryViewModel(const FMALogEntry& Entry, const UMAUITheme* Theme) const
{
    FMALogEntryViewModel Model;
    const FString TimeStr = Entry.Timestamp.ToString(TEXT("%H:%M:%S"));
    Model.FormattedMessage = FString::Printf(TEXT("[%s] %s"), *TimeStr, *Entry.Message);
    if (Theme)
    {
        Model.TextColor = Entry.bIsError ? Theme->DangerColor : Theme->SecondaryTextColor;
    }
    else
    {
        Model.TextColor = Entry.bIsError ? FLinearColor(1.0f, 0.3f, 0.3f, 1.0f) : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
    }
    return Model;
}

FLinearColor FMASystemLogPanelCoordinator::BuildSectionBackgroundColor(const UMAUITheme* Theme) const
{
    if (!Theme)
    {
        return FLinearColor(0.15f, 0.15f, 0.18f, 1.0f);
    }

    FLinearColor SectionBgColor = Theme->BackgroundColor;
    SectionBgColor.R += 0.05f;
    SectionBgColor.G += 0.05f;
    SectionBgColor.B += 0.05f;
    return SectionBgColor;
}
