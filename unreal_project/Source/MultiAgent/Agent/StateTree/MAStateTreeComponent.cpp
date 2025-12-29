// MAStateTreeComponent.cpp

#include "MAStateTreeComponent.h"
#include "StateTree.h"
#include "StateTreeExecutionContext.h"
#include "../../Core/Config/MAConfigManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMAStateTree, Log, All);

UMAStateTreeComponent::UMAStateTreeComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 默认启用，BeginPlay 时根据配置决定
    bStateTreeEnabled = true;
}

void UMAStateTreeComponent::BeginPlay()
{
    // 在 BeginPlay 时检查配置，决定是否启用 StateTree
    bStateTreeEnabled = ShouldUseStateTree();
    
    if (!bStateTreeEnabled)
    {
        // 禁用 StateTree：阻止自动启动
        SetStartLogicAutomatically(false);
        
        UE_LOG(LogMAStateTree, Log, TEXT("[%s] StateTree DISABLED by config (use_state_tree=false)"), 
            GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
        
        // 必须调用父类 BeginPlay，否则组件的 HasBegunPlay() 会返回 false
        // 但先禁用自动启动，这样父类 BeginPlay 不会启动 StateTree
        Super::BeginPlay();
        
        // BeginPlay 后再停止逻辑（如果有的话）
        StopLogic(TEXT("StateTree disabled by config"));
    }
    else
    {
        UE_LOG(LogMAStateTree, Log, TEXT("[%s] StateTree ENABLED"), 
            GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
        
        Super::BeginPlay();
    }
}

bool UMAStateTreeComponent::ShouldUseStateTree() const
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UMAConfigManager* ConfigMgr = GI->GetSubsystem<UMAConfigManager>())
            {
                return ConfigMgr->bUseStateTree;
            }
        }
    }
    // 默认启用
    return true;
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
    
    StateTreeRef.SetStateTree(InStateTree);
    
    if (!StateTreeRef.IsValid()) return;
    
    // 延迟启动 - 等待下一帧
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick([this]()
        {
            StopLogic(TEXT("Resetting for new StateTree"));
            StartLogic();
        });
    }
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
    
    // 如果有有效的 StateTree，启动它
    if (StateTreeRef.IsValid())
    {
        StartLogic();
    }
    
    UE_LOG(LogMAStateTree, Log, TEXT("[%s] StateTree enabled manually"), 
        GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}
