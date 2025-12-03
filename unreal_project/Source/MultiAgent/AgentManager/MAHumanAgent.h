#pragma once

#include "CoreMinimal.h"
#include "MAAgent.h"
#include "MAHumanAgent.generated.h"

UCLASS()
class MULTIAGENT_API AMAHumanAgent : public AMAAgent
{
    GENERATED_BODY()

public:
    AMAHumanAgent();

protected:
    virtual void BeginPlay() override;
    
    // 重写导航 Tick，为动画蓝图提供加速度输入
    virtual void OnNavigationTick() override;
};
