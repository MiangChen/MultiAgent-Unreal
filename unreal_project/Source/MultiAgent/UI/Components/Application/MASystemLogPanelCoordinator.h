#pragma once

#include "CoreMinimal.h"
#include "../Domain/MASystemLogPanelModels.h"

struct FMALogEntry;
class UMAUITheme;

class FMASystemLogPanelCoordinator
{
public:
    void AppendLog(TArray<FMALogEntry>& Entries, const FString& Message, bool bIsError, int32 MaxEntries) const;
    void ClearLogs(TArray<FMALogEntry>& Entries) const;
    FMALogEntryViewModel BuildEntryViewModel(const FMALogEntry& Entry, const UMAUITheme* Theme) const;
    FLinearColor BuildSectionBackgroundColor(const UMAUITheme* Theme) const;
};
