// MAInputComponent.h
// 增强输入组件 - 封装 Enhanced Input 绑定逻辑

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "MAInputConfig.h"
#include "MAInputComponent.generated.h"

/**
 * 增强输入组件
 * 提供便捷的输入绑定方法
 */
UCLASS(Config = Input)
class MULTIAGENT_API UMAInputComponent : public UEnhancedInputComponent
{
    GENERATED_BODY()

public:
    UMAInputComponent(const FObjectInitializer& ObjectInitializer);

    /**
     * 绑定原生输入动作
     * @param InputConfig 输入配置
     * @param Object 绑定对象
     * @param PressedFunc 按下回调
     * @param ReleasedFunc 释放回调
     */
    template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
    void BindNativeAction(const UMAInputConfig* InputConfig, const FGameplayTag& InputTag, 
        ETriggerEvent TriggerEvent, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc);

    /**
     * 绑定技能输入动作
     * @param InputConfig 输入配置
     * @param Object 绑定对象
     * @param PressedFunc 按下回调
     * @param ReleasedFunc 释放回调
     */
    template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
    void BindAbilityActions(const UMAInputConfig* InputConfig, UserClass* Object, 
        PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc);
};

// 模板实现
template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UMAInputComponent::BindNativeAction(const UMAInputConfig* InputConfig, const FGameplayTag& InputTag,
    ETriggerEvent TriggerEvent, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc)
{
    check(InputConfig);

    if (const UInputAction* IA = InputConfig->FindInputActionByTag(InputTag))
    {
        if (PressedFunc)
        {
            BindAction(IA, TriggerEvent, Object, PressedFunc);
        }

        if (ReleasedFunc)
        {
            BindAction(IA, ETriggerEvent::Completed, Object, ReleasedFunc);
        }
    }
}

template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UMAInputComponent::BindAbilityActions(const UMAInputConfig* InputConfig, UserClass* Object,
    PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc)
{
    check(InputConfig);

    for (const FMAInputAction& Action : InputConfig->AbilityInputActions)
    {
        if (Action.InputAction && Action.InputTag.IsValid())
        {
            if (PressedFunc)
            {
                BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag);
            }

            if (ReleasedFunc)
            {
                BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag);
            }
        }
    }
}
