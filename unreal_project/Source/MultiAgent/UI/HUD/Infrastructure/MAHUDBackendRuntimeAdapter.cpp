// L4 runtime bridge for HUD/backend communication subsystem access.

#include "MAHUDBackendRuntimeAdapter.h"

#include "../Runtime/MAHUD.h"
#include "../../../Core/Comm/Runtime/MACommSubsystem.h"
#include "../../../Core/TempData/Runtime/MATempDataManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UMACommSubsystem* FMAHUDBackendRuntimeAdapter::ResolveCommSubsystem(UWorld* World) const
{
    if (UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr)
    {
        return GameInstance->GetSubsystem<UMACommSubsystem>();
    }

    return nullptr;
}

bool FMAHUDBackendRuntimeAdapter::BindPlannerResponse(UWorld* World, AMAHUD* HUD) const
{
    UMACommSubsystem* CommSubsystem = ResolveCommSubsystem(World);
    if (!CommSubsystem || !HUD)
    {
        return false;
    }

    if (!CommSubsystem->OnPlannerResponse.IsAlreadyBound(HUD, &AMAHUD::OnPlannerResponse))
    {
        CommSubsystem->OnPlannerResponse.AddDynamic(HUD, &AMAHUD::OnPlannerResponse);
    }

    return true;
}

bool FMAHUDBackendRuntimeAdapter::BindBackendEvents(UWorld* World, AMAHUD* HUD) const
{
    UMACommSubsystem* CommSubsystem = ResolveCommSubsystem(World);
    if (!CommSubsystem || !HUD)
    {
        return false;
    }

    if (!CommSubsystem->OnTaskPlanReceived.IsAlreadyBound(HUD, &AMAHUD::OnTaskGraphReceived))
    {
        CommSubsystem->OnTaskPlanReceived.AddDynamic(HUD, &AMAHUD::OnTaskGraphReceived);
    }

    if (!CommSubsystem->OnSkillAllocationReceived.IsAlreadyBound(HUD, &AMAHUD::OnSkillAllocationReceived))
    {
        CommSubsystem->OnSkillAllocationReceived.AddDynamic(HUD, &AMAHUD::OnSkillAllocationReceived);
    }

    if (!CommSubsystem->OnSkillListReceived.IsAlreadyBound(HUD, &AMAHUD::OnSkillListReceived))
    {
        CommSubsystem->OnSkillListReceived.AddDynamic(HUD, &AMAHUD::OnSkillListReceived);
    }

    return true;
}

bool FMAHUDBackendRuntimeAdapter::SendNaturalLanguageCommand(UWorld* World, AMAHUD* HUD, const FString& Command) const
{
    if (!BindPlannerResponse(World, HUD))
    {
        return false;
    }

    if (UMACommSubsystem* CommSubsystem = ResolveCommSubsystem(World))
    {
        CommSubsystem->SendNaturalLanguageCommand(Command);
        return true;
    }

    return false;
}

bool FMAHUDBackendRuntimeAdapter::SendTaskGraphSubmit(UWorld* World, const FString& TaskGraphJson) const
{
    if (UMACommSubsystem* CommSubsystem = ResolveCommSubsystem(World))
    {
        CommSubsystem->SendTaskGraphSubmitMessage(TaskGraphJson);
        return true;
    }

    return false;
}

bool FMAHUDBackendRuntimeAdapter::SendReviewResponse(
    UWorld* World,
    const bool bApproved,
    const FString& DataJson,
    const FString& RejectionReason,
    const FString& OriginalMessageId) const
{
    if (UMACommSubsystem* CommSubsystem = ResolveCommSubsystem(World))
    {
        CommSubsystem->SendReviewResponseSimple(bApproved, DataJson, RejectionReason, OriginalMessageId);
        return true;
    }

    return false;
}

bool FMAHUDBackendRuntimeAdapter::SendButtonEvent(
    UWorld* World,
    const FString& WidgetName,
    const FString& EventType,
    const FString& Label) const
{
    if (UMACommSubsystem* CommSubsystem = ResolveCommSubsystem(World))
    {
        CommSubsystem->SendButtonEventMessage(WidgetName, EventType, Label);
        return true;
    }

    return false;
}

FString FMAHUDBackendRuntimeAdapter::GetTaskGraphReviewMessageId(UWorld* World) const
{
    if (UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr)
    {
        if (UMATempDataManager* TempDataManager = GameInstance->GetSubsystem<UMATempDataManager>())
        {
            return TempDataManager->GetTaskGraphReviewMessageId();
        }
    }

    return FString();
}

FString FMAHUDBackendRuntimeAdapter::GetSkillAllocationReviewMessageId(UWorld* World) const
{
    if (UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr)
    {
        if (UMATempDataManager* TempDataManager = GameInstance->GetSubsystem<UMATempDataManager>())
        {
            return TempDataManager->GetSkillAllocationReviewMessageId();
        }
    }

    return FString();
}
