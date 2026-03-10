#pragma once

#include "CoreMinimal.h"
#include "Core/SkillAllocation/Domain/MASkillAllocationTypes.h"

class UObject;
class UMASkillAllocationViewer;

class FMASkillAllocationViewerRuntimeAdapter
{
public:
    static bool TryLoadSkillAllocation(const UObject* WorldContextObject, FMASkillAllocationData& OutData, FString& OutError);
    static bool TrySaveSkillAllocation(const UObject* WorldContextObject, const FMASkillAllocationData& Data, FString& OutError);
    static bool TryNavigateToSkillAllocationModal(const UObject* WorldContextObject, FString& OutError);
    static bool TryHideSkillAllocationViewer(const UObject* WorldContextObject, FString& OutError);
    static bool BindSkillAllocationChanged(const UObject* WorldContextObject, UMASkillAllocationViewer* Viewer, FString& OutError);
    static bool BindSkillStatusUpdated(const UObject* WorldContextObject, UMASkillAllocationViewer* Viewer, FString& OutError);
};
