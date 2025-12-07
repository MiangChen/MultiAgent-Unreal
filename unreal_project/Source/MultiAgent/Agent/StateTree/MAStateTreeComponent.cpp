// MAStateTreeComponent.cpp

#include "MAStateTreeComponent.h"
#include "StateTree.h"

UMAStateTreeComponent::UMAStateTreeComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    UE_LOG(LogTemp, Warning, TEXT("[MAStateTreeComponent] Constructor called"));
}

void UMAStateTreeComponent::SetStateTreeAsset(UStateTree* InStateTree)
{
    if (InStateTree)
    {
        StateTreeRef.SetStateTree(InStateTree);
        UE_LOG(LogTemp, Warning, TEXT("[MAStateTreeComponent] StateTree set: %s"), *InStateTree->GetName());
        
        // 检查是否设置成功
        if (StateTreeRef.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[MAStateTreeComponent] StateTreeRef is now VALID"));
            
            // 检查 Owner
            AActor* Owner = GetOwner();
            if (Owner)
            {
                UE_LOG(LogTemp, Warning, TEXT("[MAStateTreeComponent] Owner: %s"), *Owner->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[MAStateTreeComponent] Owner is NULL!"));
            }
            
            // 检查 AIController (StateTree AI Component Schema 需要)
            APawn* Pawn = Cast<APawn>(Owner);
            if (Pawn)
            {
                AController* Controller = Pawn->GetController();
                if (Controller)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[MAStateTreeComponent] Controller: %s"), *Controller->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[MAStateTreeComponent] Pawn has NO Controller! StateTree needs AIController."));
                }
            }
            
            // 延迟启动 - 等待下一帧，确保所有组件都初始化完成
            UE_LOG(LogTemp, Warning, TEXT("[MAStateTreeComponent] Scheduling StartLogic for next tick..."));
            GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
            {
                UE_LOG(LogTemp, Warning, TEXT("[MAStateTreeComponent] Calling StartLogic() (next tick)..."));
                StartLogic();
                
                EStateTreeRunStatus Status = GetStateTreeRunStatus();
                FString StatusStr;
                switch(Status)
                {
                    case EStateTreeRunStatus::Running: StatusStr = TEXT("Running"); break;
                    case EStateTreeRunStatus::Failed: StatusStr = TEXT("Failed"); break;
                    case EStateTreeRunStatus::Succeeded: StatusStr = TEXT("Succeeded"); break;
                    case EStateTreeRunStatus::Stopped: StatusStr = TEXT("Stopped"); break;
                    default: StatusStr = TEXT("Unknown"); break;
                }
                UE_LOG(LogTemp, Warning, TEXT("[MAStateTreeComponent] After StartLogic - RunStatus: %s"), *StatusStr);
            });
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[MAStateTreeComponent] StateTreeRef is still INVALID after SetStateTree!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MAStateTreeComponent] SetStateTreeAsset called with NULL!"));
    }
}

bool UMAStateTreeComponent::HasStateTree() const
{
    return StateTreeRef.IsValid();
}
