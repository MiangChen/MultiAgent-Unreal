// MAFire.h
// 火焰特效环境对象

#pragma once

#include "CoreMinimal.h"
#include "../MAEnvironmentEffect.h"
#include "MAFire.generated.h"

class UParticleSystemComponent;

/**
 * 火焰特效环境对象
 * 
 * 继承 AMAEnvironmentEffect，在 Configure 后生成火焰粒子特效。
 * 
 * 配置示例:
 * {
 *     "label": "Fire_1",
 *     "type": "fire",
 *     "position": [6000, 8000, 0],
 *     "features": { "scale": "8" }
 * }
 * 
 * 使用的特效资源:
 * - /Game/VisualEffect/Fire/Particles/P_Fire (Cascade)
 * - /Game/VisualEffect/Fire/Particles/VFX_fire_100cm_loop (备选)
 */
UCLASS()
class MULTIAGENT_API AMAFire : public AMAEnvironmentEffect
{
    GENERATED_BODY()

public:
    AMAFire();

    /** 熄灭火焰 */
    UFUNCTION(BlueprintCallable, Category = "Fire")
    void Extinguish();

    /** 是否已熄灭 */
    UPROPERTY(BlueprintReadOnly, Category = "Fire")
    bool bIsExtinguished = false;

protected:
    virtual void SpawnEffect() override;

    //=========================================================================
    // 组件
    //=========================================================================

    /** 火焰粒子组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UParticleSystemComponent> FireParticle;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 火焰特效资源路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString FireEffectPath = TEXT("/Game/VisualEffect/Fire/Particles/P_Fire.P_Fire");

    /** 备选火焰特效路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString FallbackFireEffectPath = TEXT("/Game/VisualEffect/Fire/Particles/VFX_fire_100cm_loop.VFX_fire_100cm_loop");
};
