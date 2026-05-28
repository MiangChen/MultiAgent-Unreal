// MAStateTreeComponent.cpp

#include "MAStateTreeComponent.h"
#include "Agent/StateTree/Application/MAStateTreeUseCases.h"
#include "Agent/StateTree/Bootstrap/MAStateTreeBootstrap.h"
#include "Agent/StateTree/Infrastructure/MAStateTreeRuntimeBridge.h"
#include "StateTree.h"
#include "StateTreeExecutionContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAStateTree, Log, All);

UMAStateTreeComponent::UMAStateTreeComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 默认启用，BeginPlay 时根据配置决定
    bStateTreeEnabled = true;
}

void UMAStateTreeComponent::BeginPlay()
{
    bStateTreeEnabled = FMAStateTreeBootstrap::ShouldUseStateTree(GetWorld());
    const FMAStateTreeLifecycleFeedback LifecycleFeedback =
        FMAStateTreeUseCases::BuildBeginPlayFeedback(bStateTreeEnabled, StateTreeRef.IsValid());
    
    if (!bStateTreeEnabled)
    {
        // 禁用 StateTree：阻止自动启动
        SetStartLogicAutomatically(false);
        
        UE_LOG(LogMAStateTree, Log, TEXT("[%s] %s"),
            GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"),
            *LifecycleFeedback.Message);
        
        // 必须调用父类 BeginPlay，否则组件的 HasBegunPlay() 会返回 false
        // 但先禁用自动启动，这样父类 BeginPlay 不会启动 StateTree
        Super::BeginPlay();
        
        // BeginPlay 后再停止逻辑（如果有的话）
        StopLogic(TEXT("StateTree disabled by config"));
    }
    else
    {
        UE_LOG(LogMAStateTree, Log, TEXT("[%s] %s"),
            GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"),
            *LifecycleFeedback.Message);

        // 如果尚未设置 StateTree 资产，先关闭自动启动，避免父类 BeginPlay 在空资产上触发校验错误。
        // 资产后续通过 SetStateTreeAsset 注入后会在下一帧主动 StartLogic。
        if (!LifecycleFeedback.bShouldStartLogic)
        {
            SetStartLogicAutomatically(false);
        }

        Super::BeginPlay();
    }
}

void UMAStateTreeComponent::SetStateTreeAsset(UStateTree* InStateTree)
{
    // 如果 StateTree 被禁用，不加载资产
    if (!bStateTreeEnabled)
    {
        UE_LOG(LogMAStateTree, Log, TEXT("[%s] SetStateTreeAsset ignored - StateTree disabled"), 
            GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
        return;
    }
    
    if (!InStateTree) return;
    
    FMAStateTreeBootstrap::RestartWithAsset(*this, InStateTree);
}

bool UMAStateTreeComponent::HasStateTree() const
{
    return bStateTreeEnabled && StateTreeRef.IsValid();
}

void UMAStateTreeComponent::DisableStateTree()
{
    if (!bStateTreeEnabled) return;
    
    bStateTreeEnabled = false;
    SetStartLogicAutomatically(false);
    StopLogic(TEXT("StateTree disabled manually"));
    
    UE_LOG(LogMAStateTree, Log, TEXT("[%s] StateTree disabled manually"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}

void UMAStateTreeComponent::EnableStateTree()
{
    if (bStateTreeEnabled) return;
    
    bStateTreeEnabled = true;
    const FMAStateTreeLifecycleFeedback EnableFeedback =
        FMAStateTreeUseCases::BuildEnableFeedback(StateTreeRef.IsValid());
    
    // 如果有有效的 StateTree，启动它
    if (EnableFeedback.bShouldStartLogic)
    {
        StartLogic();
    }
    
    UE_LOG(LogMAStateTree, Log, TEXT("[%s] %s"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"),
        *EnableFeedback.Message);
}

bool UMAStateTreeComponent::SetResolvedStateTree(UStateTree* InStateTree)
{
    if (!InStateTree)
    {
        return false;
    }

    StateTreeRef.SetStateTree(InStateTree);
    return StateTreeRef.IsValid();
}

void UMAStateTreeComponent::RestartLogicNextTick()
{
    FMAStateTreeRuntimeBridge::RestartLogicNextTick(*this);
}
