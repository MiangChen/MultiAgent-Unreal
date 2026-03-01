// MASceneGraphCommandPortComposer.h
// 组装 SceneGraph CommandPort（将 manager 内部操作绑定为 command handlers）

#pragma once

#include "CoreMinimal.h"

class IMASceneGraphCommandPort;
class UMASceneGraphManager;

class FMASceneGraphCommandPortComposer
{
public:
    static TSharedPtr<IMASceneGraphCommandPort> Create(UMASceneGraphManager& Manager);
};
