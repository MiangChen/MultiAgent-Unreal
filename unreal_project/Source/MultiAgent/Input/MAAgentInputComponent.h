// MAAgentInputComponent.h
// Agent 直接控制输入组件 - 处理 Agent View Mode 下的 WASD/视角输入
// 职责单一：仅在 Agent View Mode 时激活，处理移动和视角控制

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MAAgentInputComponent.generated.h"

struct FInputActionValue;
class AMACharacter;
class APlayerController;

/**
 * Agent 输入组件
 * 
 * 生命周期:
 * - 由 ViewportManager 在进入 Agent View Mode 时创建
 * - 退出时销毁
 * 
 * 职责:
 * - 添加/移除 IMC_AgentControl
 * - 处理 WASD 移动、鼠标视角、垂直移动
 */
UCLASS()
class MULTIAGENT_API UMAAgentInputComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMAAgentInputComponent();

    // 初始化：绑定输入并添加 IMC_AgentControl
    void Initialize(APlayerController* PC, AMACharacter* Agent);
    
    // 清理：解绑输入并移除 IMC_AgentControl
    void Cleanup();

protected:
    // 输入处理
    void OnMoveInput(const FInputActionValue& Value);
    void OnLookInput(const FInputActionValue& Value);
    void OnMoveUp(const FInputActionValue& Value);
    void OnMoveDown(const FInputActionValue& Value);
    void OnLookArrowInput(const FInputActionValue& Value);
    void OnJump(const FInputActionValue& Value);

private:
    UPROPERTY()
    TWeakObjectPtr<APlayerController> OwningPC;
    
    UPROPERTY()
    TWeakObjectPtr<AMACharacter> ControlledAgent;
    
    // 相机俯仰角状态
    float CurrentCameraPitch = -15.f;
    float MinCameraPitch = -60.f;
    float MaxCameraPitch = 30.f;
    float LookSensitivityYaw = 14.0f;
    float LookSensitivityPitch = 14.0f;
};
