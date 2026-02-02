// MAEnvironmentEffect.h
// 环境特效基类 - 火焰、烟雾、强风等静态特效的基类

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IMAEnvironmentObject.h"
#include "MAEnvironmentEffect.generated.h"

struct FMAEnvironmentObjectConfig;

/**
 * 环境特效基类
 * 
 * 继承 AActor，实现 IMAEnvironmentObject 接口。
 * 作为火焰、烟雾、强风等静态特效的基类。
 * 
 * 特点:
 * - 不包含导航组件 (静态特效)
 * - 支持 scale (尺寸) 和 intensity (强度) 配置
 * - 子类负责生成具体的粒子特效
 * 
 * 配置示例:
 * {
 *     "features": {
 *         "scale": "0.5",       // 尺寸缩放 (默认 1.0)
 *         "intensity": "2.0"    // 强度/浓度 (默认 1.0)
 *     }
 * }
 * 
 * 子类:
 * - AMAFire: 火焰特效
 * - AMASmoke: 烟雾特效
 * - AMAWind: 强风特效
 */
UCLASS(Abstract)
class MULTIAGENT_API AMAEnvironmentEffect : public AActor, public IMAEnvironmentObject
{
    GENERATED_BODY()

public:
    AMAEnvironmentEffect();

    //=========================================================================
    // IMAEnvironmentObject 接口实现
    //=========================================================================

    /** 获取对象标签 */
    virtual FString GetObjectLabel() const override;

    /** 获取对象类型 */
    virtual FString GetObjectType() const override;

    /** 获取对象特征 */
    virtual const TMap<FString, FString>& GetObjectFeatures() const override;

    //=========================================================================
    // 配置方法
    //=========================================================================

    /**
     * 根据配置初始化特效
     * @param Config 环境对象配置
     */
    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void Configure(const FMAEnvironmentObjectConfig& Config);

protected:
    virtual void BeginPlay() override;

    /**
     * 生成特效 - 子类必须实现
     * 在 BeginPlay 中调用
     */
    virtual void SpawnEffect();

    //=========================================================================
    // 环境对象属性
    //=========================================================================

    /** 对象标签 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectLabel;

    /** 对象类型 (fire/smoke/wind) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    FString ObjectType;

    /** 对象特征 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
    TMap<FString, FString> Features;

    //=========================================================================
    // 特效参数
    //=========================================================================

    /** 特效尺寸缩放 (默认 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float EffectScale = 1.0f;

    /** 特效强度/浓度 (默认 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float EffectIntensity = 1.0f;

    //=========================================================================
    // 组件
    //=========================================================================

    /** 场景根组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;
};
