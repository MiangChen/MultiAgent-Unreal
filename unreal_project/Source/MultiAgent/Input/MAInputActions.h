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

    // ========== Input Mapping Context ==========
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

private:
    static UMAInputActions* Instance;

    // 创建单个 Input Action
    UInputAction* CreateInputAction(const FName& Name, EInputActionValueType ValueType = EInputActionValueType::Boolean);

    // 添加按键映射
    void AddKeyMapping(UInputMappingContext* Context, UInputAction* Action, FKey Key);
};
