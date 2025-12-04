// MAInputConfig.cpp

#include "MAInputConfig.h"
#include "InputAction.h"

const UInputAction* UMAInputConfig::FindInputActionByTag(const FGameplayTag& InputTag) const
{
    // 先在原生输入中查找
    for (const FMAInputAction& Action : NativeInputActions)
    {
        if (Action.InputAction && Action.InputTag == InputTag)
        {
            return Action.InputAction;
        }
    }

    // 再在技能输入中查找
    for (const FMAInputAction& Action : AbilityInputActions)
    {
        if (Action.InputAction && Action.InputTag == InputTag)
        {
            return Action.InputAction;
        }
    }

    return nullptr;
}
