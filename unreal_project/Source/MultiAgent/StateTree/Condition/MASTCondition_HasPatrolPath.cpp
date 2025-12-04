// MASTCondition_HasPatrolPath.cpp

#include "MASTCondition_HasPatrolPath.h"
#include "../../Actor/MAPatrolPath.h"
#include "StateTreeExecutionContext.h"
#include "Kismet/GameplayStatics.h"

bool FMASTCondition_HasPatrolPath::TestCondition(FStateTreeExecutionContext& Context) const
{
    UWorld* World = Context.GetWorld();
    if (!World)
    {
        return false;
    }
    
    // 通过 Tag 查找 PatrolPath
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(World, PatrolPathTag, FoundActors);
    
    for (AActor* Actor : FoundActors)
    {
        if (AMAPatrolPath* Path = Cast<AMAPatrolPath>(Actor))
        {
            if (Path->IsValidPath())
            {
                return true;
            }
        }
    }
    
    return false;
}
