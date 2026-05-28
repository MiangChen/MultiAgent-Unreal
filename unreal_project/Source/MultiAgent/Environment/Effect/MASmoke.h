// MASmoke.h
// 烟雾特效环境对象

#pragma once

#include "CoreMinimal.h"
#include "../MAEnvironmentEffect.h"
#include "MASmoke.generated.h"

class UParticleSystemComponent;

/**
 * 烟雾特效环境对象
 * 
 * 继承 AMAEnvironmentEffect，在 Configure 后生成烟雾粒子特效。
 * 使用 VisualEffect/Wind 资产包的 P_wind_dust 系列实现扬尘/烟雾效果。
 * 
 * 配置示例:
 * {
 *     "label": "Smoke_1",
 *     "type": "smoke",
 *     "position": [-10500, 1900, 600],
 *     "features": { "scale": "1.0" }
 * }
 * 
 * 使用的特效资源:
 * - /Game/VisualEffect/Wind/Particles/P_wind_dust (Cascade)
 * - /Game/VisualEffect/Wind/Particles/P_wind_dust_2 (备选)
 */
UCLASS()
class MULTIAGENT_API AMASmoke : public AMAEnvironmentEffect
{
    GENERATED_BODY()

public:
    AMASmoke();

protected:
    virtual void SpawnEffect() override;

    //=========================================================================
    // 组件
    //=========================================================================

    /** 烟雾粒子组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UParticleSystemComponent> SmokeParticle;

    //=========================================================================
    // 配置
    //=========================================================================

    /** 烟雾特效资源路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString SmokeEffectPath = TEXT("/Game/VisualEffect/Wind/Particles/P_wind_dust.P_wind_dust");

    /** 备选烟雾特效路径 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FString FallbackSmokeEffectPath = TEXT("/Game/VisualEffect/Wind/Particles/P_wind_dust_2.P_wind_dust_2");
};
