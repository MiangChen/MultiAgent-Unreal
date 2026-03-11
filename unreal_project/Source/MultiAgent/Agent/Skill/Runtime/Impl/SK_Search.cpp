#include "SK_Search.h"

#include "Agent/Skill/Application/MASkillCompletionUseCases.h"
#include "Agent/Skill/Domain/MASkillTags.h"
#include "Agent/Skill/Infrastructure/MASkillPIPCameraBridge.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"

USK_Search::USK_Search()
{
    ActivationOwnedTags.AddTag(FMASkillTags::Get().Status_Searching);
    bSearchSucceeded = false;
    SearchResultMessage = TEXT("");
}

void USK_Search::ResetSearchRuntimeState()
{
    CurrentWaypointIndex = 0;
    LastUIUpdateIndex = -1;
    PatrolCycleCount = 0;
    ConsecutiveNavFailures = 0;
    bSearchSucceeded = false;
    SearchResultMessage = TEXT("");
}

bool USK_Search::InitializeSearchContext(AMACharacter& Character, UMASkillComponent& SkillComp)
{
    NavigationService = Character.GetNavigationService();
    if (!NavigationService.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[SK_Search] %s: NavigationService not found"), *Character.AgentLabel);
        SearchResultMessage = TEXT("Search failed: NavigationService not available");
        return false;
    }

    const FMASkillParams& Params = SkillComp.GetSkillParams();
    ScanWidth = Params.SearchScanWidth > 0.f ? Params.SearchScanWidth : 800.f;
    CurrentSearchMode = Params.SearchMode;
    PatrolWaitTime = Params.PatrolWaitTime > 0.f ? Params.PatrolWaitTime : 2.0f;
    PatrolCycleLimit = Params.PatrolCycleLimit > 0 ? Params.PatrolCycleLimit : 1;

    const FMAFeedbackContext& Context = SkillComp.GetFeedbackContext();
    FoundTargetLocations = Context.FoundLocations;
    bHasFoundTarget = FoundTargetLocations.Num() > 0;

    if (bHasFoundTarget)
    {
        UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: Found %d targets, will end early when reaching nearest target"),
            *Character.AgentLabel, FoundTargetLocations.Num());
    }

    GenerateSearchPath();

    if (!FMASkillPIPCameraBridge::IsAvailable(Character.GetWorld()))
    {
        UE_LOG(LogTemp, Warning, TEXT("[SK_Search] %s: PIP camera manager not available"), *Character.AgentLabel);
    }

    if (SearchPath.Num() == 0)
    {
        SearchResultMessage = TEXT("Search failed: No valid search path generated");
        Character.ShowStatus(TEXT("[Search] No valid path"), 2.f);
        return false;
    }

    return true;
}

void USK_Search::CompleteSearch(const bool bSuccess, const FString& Message, const bool bCancelAbility)
{
    bSearchSucceeded = bSuccess;
    SearchResultMessage = Message;
    EndAbility(CachedHandle, GetCurrentActorInfo(), CachedActivationInfo, true, bCancelAbility);
}

void USK_Search::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CachedHandle = Handle;
    CachedActivationInfo = ActivationInfo;
    ResetSearchRuntimeState();

    AMACharacter* Character = GetOwningCharacter();
    if (!Character)
    {
        CompleteSearch(false, TEXT("Search failed: Character not found"), true);
        return;
    }

    UMASkillComponent* SkillComp = Character->GetSkillComponent();
    if (!SkillComp)
    {
        CompleteSearch(false, TEXT("Search failed: SkillComponent not found"), true);
        return;
    }

    if (!InitializeSearchContext(*Character, *SkillComp))
    {
        CompleteSearch(false, SearchResultMessage, true);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s (%s): Starting %s search with %d waypoints%s"),
        *Character->AgentLabel,
        *GetRobotTypeLabel(),
        CurrentSearchMode == ESearchMode::Coverage ? TEXT("Coverage") : TEXT("Patrol"),
        SearchPath.Num(),
        CurrentSearchMode == ESearchMode::Patrol ? *FString::Printf(TEXT(", %d cycles"), PatrolCycleLimit) : TEXT(""));

    Character->ShowAbilityStatus(TEXT("Search"), FString::Printf(TEXT("0/%d waypoints"), SearchPath.Num()));
    Character->bIsMoving = true;

    NavigationService->OnNavigationCompleted.AddDynamic(this, &USK_Search::OnNavigationCompleted);

    if (CurrentSearchMode == ESearchMode::Coverage)
    {
        CreateCoverageCamera();
    }

    NavigateToNextWaypoint();
}

void USK_Search::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    AMACharacter* Character = GetOwningCharacter();

    bool bSuccessToNotify = bSearchSucceeded;
    FString MessageToNotify = SearchResultMessage;

    CleanupSearchRuntime(Character, bWasCancelled, bSuccessToNotify, MessageToNotify);

    UE_LOG(LogTemp, Log, TEXT("[SK_Search] %s: EndAbility - Mode=%s, Success=%s, Cycles=%d, Message=%s"),
        Character ? *Character->AgentLabel : TEXT("NULL"),
        CurrentSearchMode == ESearchMode::Coverage ? TEXT("Coverage") : TEXT("Patrol"),
        bSuccessToNotify ? TEXT("true") : TEXT("false"),
        PatrolCycleCount,
        *MessageToNotify);

    SearchPath.Empty();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

    if (Character)
    {
        if (UMASkillComponent* SkillComp = Character->GetSkillComponent())
        {
            FMASkillCompletionUseCases::NotifyAbilityFinished(*SkillComp, EMACommand::Search, bSuccessToNotify, MessageToNotify);
        }
    }
}
