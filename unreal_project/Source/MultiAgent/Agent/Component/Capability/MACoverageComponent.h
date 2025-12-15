// MACoverageComponent.h
// 覆盖扫描能力组件 - 实现 IMACoverable 接口
// 存储覆盖区域参数

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../../Interface/MAAgentInterfaces.h"
#include "MACoverageComponent.generated.h"

/**
 * 覆盖扫描能力组件 - 管理 Agent 的区域覆盖参数
 */
UCLASS(ClassGroup=(MultiAgent), meta=(BlueprintSpawnableComponent))
class MULTIAGENT_API UMACoverageComponent : public UActorComponent, public IMACoverable
{
    GENERATED_BODY()

public:
    UMACoverageComponent();

    // ========== IMACoverable Interface ==========
    virtual AActor* GetCoverageArea() const override { return CoverageArea.Get(); }
    virtual void SetCoverageArea(AActor* Area) override { CoverageArea = Area; }
    virtual float GetScanRadius() const override { return ScanRadius; }

    // ========== 属性 ==========
    
    // 覆盖区域 Actor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
    TWeakObjectPtr<AActor> CoverageArea;

    // 扫描间距半径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coverage")
    float ScanRadius = 200.f;

    // ========== 辅助方法 ==========
    
    UFUNCTION(BlueprintCallable, Category = "Coverage")
    bool HasCoverageArea() const { return CoverageArea.IsValid(); }

    UFUNCTION(BlueprintCallable, Category = "Coverage")
    void ClearCoverageArea() { CoverageArea.Reset(); }

    UFUNCTION(BlueprintCallable, Category = "Coverage")
    void SetScanRadius(float NewRadius) { ScanRadius = NewRadius; }
};
