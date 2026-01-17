// MAUIManager.h
// UI Manager - 负责 Widget 的生命周期管理和创建
// 从 MAHUD 分离出 Widget 管理职责，降低代码复杂度

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MAUIManager.generated.h"

// 前向声明
class UMATaskPlannerWidget;
class UMASkillAllocationViewer;
class UMASimpleMainWidget;
class UMADirectControlIndicator;
class UMAEmergencyWidget;
class UMAModifyWidget;
class UMAEditWidget;
class UMASceneListWidget;
class UMAHUDWidget;
class UUserWidget;

//=============================================================================
// Widget 类型枚举
//=============================================================================

/**
 * Widget 类型枚举
 * 用于标识不同类型的 Widget，便于统一管理
 */
UENUM(BlueprintType)
enum class EMAWidgetType : uint8
{
    None            UMETA(DisplayName = "None"),
    HUD             UMETA(DisplayName = "HUD Overlay"),
    TaskPlanner     UMETA(DisplayName = "Task Planner"),
    SkillAllocation UMETA(DisplayName = "Skill Allocation"),
    SimpleMain      UMETA(DisplayName = "Simple Main (Legacy)"),
    SemanticMap     UMETA(DisplayName = "Semantic Map"),
    DirectControl   UMETA(DisplayName = "Direct Control"),
    Emergency       UMETA(DisplayName = "Emergency"),
    Modify          UMETA(DisplayName = "Modify"),
    Edit            UMETA(DisplayName = "Edit"),
    SceneList       UMETA(DisplayName = "Scene List")
};

//=============================================================================
// 日志类别
//=============================================================================
DECLARE_LOG_CATEGORY_EXTERN(LogMAUIManager, Log, All);

//=============================================================================
// UMAUIManager - Widget 管理器
//=============================================================================

/**
 * UI Manager - Widget 管理器
 * 
 * 职责:
 * - 创建所有 Widget 实例
 * - 提供 Widget 访问接口
 * - 管理 Widget 可见性状态
 * - 处理输入模式切换
 * 
 * 使用方式:
 * - 由 MAHUD 持有，生命周期与 MAHUD 一致
 * - 在 MAHUD::BeginPlay 中调用 Initialize() 和 CreateAllWidgets()
 * - 通过 GetWidget() 或类型安全 Getter 访问 Widget
 * - 通过 ShowWidget() / HideWidget() / ToggleWidget() 控制可见性
 */
UCLASS()
class MULTIAGENT_API UMAUIManager : public UObject
{
    GENERATED_BODY()

public:
    //=========================================================================
    // 初始化
    //=========================================================================

    /**
     * 初始化 UI Manager
     * @param InOwningPC 拥有此 Manager 的 PlayerController
     */
    void Initialize(APlayerController* InOwningPC);

    /**
     * 创建所有 Widget 实例
     * 必须在 Initialize() 之后调用
     */
    void CreateAllWidgets();

    //=========================================================================
    // Widget 访问 - 通用接口
    //=========================================================================

    /**
     * 获取指定类型的 Widget
     * @param Type Widget 类型
     * @return Widget 指针，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    UUserWidget* GetWidget(EMAWidgetType Type) const;

    //=========================================================================
    // Widget 访问 - 类型安全 Getter
    //=========================================================================

    /** 获取 TaskPlannerWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMATaskPlannerWidget* GetTaskPlannerWidget() const;

    /** 获取 SkillAllocationViewer 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMASkillAllocationViewer* GetSkillAllocationViewer() const;

    /** 获取 SimpleMainWidget 实例 (Legacy) */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMASimpleMainWidget* GetSimpleMainWidget() const;

    /** 获取 DirectControlIndicator 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMADirectControlIndicator* GetDirectControlIndicator() const;

    /** 获取 EmergencyWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMAEmergencyWidget* GetEmergencyWidget() const;

    /** 获取 ModifyWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMAModifyWidget* GetModifyWidget() const;

    /** 获取 EditWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMAEditWidget* GetEditWidget() const;

    /** 获取 SceneListWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMASceneListWidget* GetSceneListWidget() const;

    /** 获取 HUDWidget 实例 */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMAHUDWidget* GetHUDWidget() const;

    //=========================================================================
    // Widget 可见性控制
    //=========================================================================

    /**
     * 显示指定类型的 Widget
     * @param Type Widget 类型
     * @param bSetFocus 是否设置焦点 (默认 true)
     * @return 是否成功显示
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    bool ShowWidget(EMAWidgetType Type, bool bSetFocus = true);

    /**
     * 隐藏指定类型的 Widget
     * @param Type Widget 类型
     * @return 是否成功隐藏
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    bool HideWidget(EMAWidgetType Type);

    /**
     * 切换指定类型 Widget 的可见性
     * @param Type Widget 类型
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleWidget(EMAWidgetType Type);

    /**
     * 检查指定类型的 Widget 是否可见
     * @param Type Widget 类型
     * @return 是否可见
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsWidgetVisible(EMAWidgetType Type) const;

    /**
     * 设置 Widget 焦点
     * @param Type Widget 类型
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetWidgetFocus(EMAWidgetType Type);

    //=========================================================================
    // SemanticMap Widget 类引用 (蓝图设置)
    //=========================================================================

    /** SemanticMap Widget 类引用 (后续阶段) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Classes")
    TSubclassOf<UUserWidget> SemanticMapWidgetClass;

private:
    //=========================================================================
    // 内部状态
    //=========================================================================

    /** 拥有此 Manager 的 PlayerController */
    UPROPERTY()
    APlayerController* OwningPC;

    /** Widget 实例映射 */
    UPROPERTY()
    TMap<EMAWidgetType, UUserWidget*> Widgets;

    //=========================================================================
    // Widget 实例 (类型安全存储)
    //=========================================================================

    UPROPERTY()
    UMATaskPlannerWidget* TaskPlannerWidget;

    UPROPERTY()
    UMASkillAllocationViewer* SkillAllocationViewer;

    UPROPERTY()
    UMASimpleMainWidget* SimpleMainWidget;

    UPROPERTY()
    UMADirectControlIndicator* DirectControlIndicator;

    UPROPERTY()
    UMAEmergencyWidget* EmergencyWidget;

    UPROPERTY()
    UMAModifyWidget* ModifyWidget;

    UPROPERTY()
    UMAEditWidget* EditWidget;

    UPROPERTY()
    UMASceneListWidget* SceneListWidget;

    UPROPERTY()
    UMAHUDWidget* HUDWidget;

    UPROPERTY()
    UUserWidget* SemanticMapWidget;

    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 获取 Widget 的 Z-Order
     * @param Type Widget 类型
     * @return Z-Order 值
     */
    int32 GetWidgetZOrder(EMAWidgetType Type) const;

    /**
     * 设置输入模式为 UI 和游戏混合
     * @param Widget 要聚焦的 Widget
     */
    void SetInputModeGameAndUI(UUserWidget* Widget);

    /**
     * 恢复输入模式为纯游戏
     */
    void SetInputModeGameOnly();
};
