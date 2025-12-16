// MASTTaskUtils.h
// 公共辅助函数，供 StateTree Task 使用

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

namespace MASTTaskUtils
{
    /**
     * 从 Actor 获取实现了指定 Interface 的 Component
     * 先尝试从 Actor 本身获取 (兼容旧代码)，再从 Component 获取
     */
    template<typename T>
    T* GetCapabilityInterface(AActor* Actor)
    {
        if (!Actor) return nullptr;
        
        // 先尝试从 Actor 本身获取 (兼容旧代码)
        if (T* Interface = Cast<T>(Actor))
        {
            return Interface;
        }
        
        // 再从 Component 获取
        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Comp : Components)
        {
            if (T* Interface = Cast<T>(Comp))
            {
                return Interface;
            }
        }
        
        return nullptr;
    }
}
