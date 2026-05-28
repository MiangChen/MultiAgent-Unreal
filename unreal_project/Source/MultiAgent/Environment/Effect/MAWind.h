// MAWind.h
// 风特效环境对象

#pragma once

#include "CoreMinimal.h"
#include "../MAEnvironmentEffect.h"
#include "MAWind.generated.h"

class UNiagaraComponent;
class UParticleSystemComponent;

/**
 * 风特效环境对象
 * 
 * 继承 AMAEnvironmentEffect，在 Configure 后生成风粒子特效。
 * 支持 Niagara 和 Cascade 两种粒子系统。
 * 
 * 配置示例:
 * {
 *     "label": "Wind_1",
 *     "type": "wind",
 *     "position": [-7000, 7000, 500],
 *     "features": { "scale": "0.5" }
 * }
 * 
 * 使用的特效资源:
 * - /Game/Map/SciFiModernCity/Particle/PS_WindWisps (主选 - Niagara)
 * - /Game/VisualEffect/Wind/Particles/P_wind_dust_2 (备选 - Cascade)
 */
UCLASS()
class MULTIAGENT_API AMAWind : public AMAEnvironmentEffect
{
    GENERATED_BODY()

public:
    AMAWind();

protected:
    virtual void SpawnEffect() override;

    //=========================================================================
    // 组件
    //=========================================================================

    /** Niagara 风粒子组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UNiagaraComponent> NiagaraWindParticle;

    /** Cascade 风粒子组件 (备选) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UParticleSystemComponent> CascadeWindParticle;

    //=========================================================================
    // 配置
    //=========================================================================

    /** Niagara 风特效资源路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString NiagaraWindEffectPath = TEXT("/Game/VisualEffect/Wind/Particles/PS_WindWisps.PS_WindWisps");

    /** Cascade 备选风特效路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString CascadeWindEffectPath = TEXT("/Game/VisualEffect/Wind/Particles/P_wind_dust_2.P_wind_dust_2");
};
