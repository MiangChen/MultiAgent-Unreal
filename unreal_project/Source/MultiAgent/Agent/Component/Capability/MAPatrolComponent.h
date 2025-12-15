// MAPatrolComponent.h
// 巡逻能力组件 - 实现 IMAPatrollable 接口
// 存储巡逻路径和扫描半径参数

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../Interface/MAAgentInterfaces.h"
#include "../../../Environment/MAPatrolPath.h"
#include "MAPatrolComponent.generated.h"

/**
 * 巡逻能力组件 - 管理 Agent 的巡逻参数
 * 
 * 使用方式:
 *   1. 在 Character 构造函数中创建
 *   2. StateTree Task 通过 Interface 获取参数
 */
UCLASS(ClassGroup=(MultiAgent), meta=(BlueprintSpawnableComponent))
class MULTIAGENT_API UMAPatrolComponent : public UActorComponent, public IMAPatrollable
{
    GENERATED_BODY()

public:
    UMAPatrolComponent();

    // ========== IMAPatrollable Interface ==========
    virtual AMAPatrolPath* GetPatrolPath() const override { return PatrolPath.Get(); }
    virtual void SetPatrolPath(AMAPatrolPath* Path) override { PatrolPath = Path; }
    virtual float GetScanRadius() const override { return ScanRadius; }

    // ========== 属性 ==========
    
    // 巡逻路径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    TWeakObjectPtr<AMAPatrolPath> PatrolPath;

    // 扫描/到达判定半径 (也用于 Follow 距离、Coverage 间距)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    float ScanRadius = 200.f;

    // ========== 辅助方法 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Patrol")
    bool HasPatrolPath() const { return PatrolPath.IsValid(); }

    UFUNCTION(BlueprintCallable, Category = "Patrol")
    void ClearPatrolPath() { PatrolPath.Reset(); }
};
