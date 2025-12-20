// MAInputActions.h
// 默认 Input Actions 定义 - 在 C++ 中创建，无需手动配置

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "MAInputActions.generated.h"

/**
 * 默认输入动作集合
 * 在 BeginPlay 时自动创建和配置
 */
UCLASS(Blueprintable, BlueprintType)
class MULTIAGENT_API UMAInputActions : public UObject
{
    GENERATED_BODY()

public:
    // 获取单例实例
    static UMAInputActions* Get();

    // 初始化所有 Input Actions
    void Initialize();

    // ========== Input Actions ==========
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_LeftClick;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_RightClick;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_MiddleClick;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Pickup;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Drop;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_SpawnItem;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_SpawnRobotDog;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_PrintInfo;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_DestroyLast;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_SwitchCamera;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ReturnSpectator;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_StartPatrol;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_StartCharge;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_StopIdle;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_StartCoverage;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_StartFollow;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_StartAvoid;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_StartFormation;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_TakePhoto;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ToggleRecording;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ToggleTCPStream;

    // ========== 编组快捷键 (星际争霸风格) ==========
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ControlGroup1;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ControlGroup2;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ControlGroup3;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ControlGroup4;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ControlGroup5;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_CreateSquad;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_DisbandSquad;

    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ToggleMouseMode;

    /** 切换主 UI 显示/隐藏 (Z 键) */
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_ToggleMainUI;

    // ========== 突发事件系统 ==========
    /** 触发/结束突发事件 ("-" 键) */
    UPROPERTY(BlueprintReadOnly, Category = "Input|Emergency")
    TObjectPtr<UInputAction> IA_TriggerEmergency;

    /** 切换突发事件详情界面 ("X" 键) */
    UPROPERTY(BlueprintReadOnly, Category = "Input|Emergency")
    TObjectPtr<UInputAction> IA_ToggleEmergencyUI;

    // ========== Agent View Mode 移动和视角控制 ==========
    /** WASD 移动输入 (Axis2D: X=左右, Y=前后) */
    UPROPERTY(BlueprintReadOnly, Category = "Input|AgentControl")
    TObjectPtr<UInputAction> IA_Move;

    /** 鼠标视角输入 (Axis2D: X=Yaw, Y=Pitch) */
    UPROPERTY(BlueprintReadOnly, Category = "Input|AgentControl")
    TObjectPtr<UInputAction> IA_Look;

    /** 上升输入 (Space 键，用于 Drone) */
    UPROPERTY(BlueprintReadOnly, Category = "Input|AgentControl")
    TObjectPtr<UInputAction> IA_MoveUp;

    /** 下降输入 (Left Ctrl 键，用于 Drone) */
    UPROPERTY(BlueprintReadOnly, Category = "Input|AgentControl")
    TObjectPtr<UInputAction> IA_MoveDown;

    /** 方向键视角输入 (Axis2D: X=Yaw, Y=Pitch) */
    UPROPERTY(BlueprintReadOnly, Category = "Input|AgentControl")
    TObjectPtr<UInputAction> IA_LookArrow;

    /** 跳跃输入 (空格键，用于地面单位跳跃) */
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> IA_Jump;

    // ========== Input Mapping Contexts ==========
    
    /** RTS 模式输入 (优先级 0, 始终激活) - 框选、编组、生成等 */
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> IMC_RTS;
    
    /** Agent Control 模式输入 (优先级 1, 仅 Agent View 时激活) - WASD、视角 */
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> IMC_AgentControl;
    
    /** 兼容旧代码：返回 IMC_RTS */
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

private:
    static UMAInputActions* Instance;

    // 创建单个 Input Action
    UInputAction* CreateInputAction(const FName& Name, EInputActionValueType ValueType = EInputActionValueType::Boolean);

    // 添加按键映射
    void AddKeyMapping(UInputMappingContext* Context, UInputAction* Action, FKey Key);

    // 添加 WASD 移动映射 (Axis2D)
    void AddWASDMapping(UInputMappingContext* Context, UInputAction* Action);

    // 添加鼠标视角映射 (Axis2D)
    void AddMouseLookMapping(UInputMappingContext* Context, UInputAction* Action);

    // 添加方向键视角映射 (Axis2D)
    void AddArrowLookMapping(UInputMappingContext* Context, UInputAction* Action);
};
