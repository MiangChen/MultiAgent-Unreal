// MAHUD.h
// HUD 管理器 - 管理所有 UI Widget 的创建、显示和隐藏
// 继承自 MASelectionHUD 以保留框选绘制功能
// Requirements: 7.1, 2.1, 2.2, 3.3

#pragma once

#include "CoreMinimal.h"
#include "MASelectionHUD.h"
#include "MAHUD.generated.h"

class UMASimpleMainWidget;
class UMATaskPlannerWidget;
class UMADirectControlIndicator;
class UMAEmergencyWidget;
class UMAModifyWidget;
class AMAPlayerController;
class AMACharacter;
class UMAEmergencyManager;
class UMACameraSensorComponent;
struct FMAPlannerResponse;
struct FMATaskGraphData;

/**
 * HUD 管理器
 * 
 * 职责:
 * - 创建和管理所有 UI Widget (MainWidget, SemanticMap, GanttChart 等)
 * - 响应 PlayerController 的 UI 切换事件
 * - 管理 Widget 的显示/隐藏状态
 * - 处理输入框焦点重置
 * 
 * Requirements: 7.1, 2.1, 2.2
 */
UCLASS()
class MULTIAGENT_API AMAHUD : public AMASelectionHUD
{
    GENERATED_BODY()

public:
    AMAHUD();

    //=========================================================================
    // Widget 类引用 (在蓝图中设置)
    //=========================================================================

    /** SemanticMap Widget 类引用 (后续阶段) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Classes")
    TSubclassOf<UUserWidget> SemanticMapWidgetClass;

    //=========================================================================
    // Widget 实例
    //=========================================================================

    /** SimpleMainWidget 实例 (纯 C++ UI 实现) - 已弃用，保留向后兼容 */
    UPROPERTY(BlueprintReadOnly, Category = "UI Instances")
    UMASimpleMainWidget* SimpleMainWidget;

    /** TaskPlannerWidget 实例 (任务规划工作台) */
    UPROPERTY(BlueprintReadOnly, Category = "UI Instances")
    UMATaskPlannerWidget* TaskPlannerWidget;

    /** SemanticMap Widget 实例 (后续阶段) */
    UPROPERTY(BlueprintReadOnly, Category = "UI Instances")
    UUserWidget* SemanticMapWidget;

    /** Direct Control 指示器实例 */
    UPROPERTY(BlueprintReadOnly, Category = "UI Instances")
    UMADirectControlIndicator* DirectControlIndicator;

    /** 突发事件详情界面实例 */
    UPROPERTY(BlueprintReadOnly, Category = "UI Instances")
    UMAEmergencyWidget* EmergencyWidget;

    /** Modify 模式修改面板实例 */
    UPROPERTY(BlueprintReadOnly, Category = "UI Instances")
    UMAModifyWidget* ModifyWidget;

    //=========================================================================
    // UI 状态
    //=========================================================================

    /** MainWidget 是否可见 */
    UPROPERTY(BlueprintReadOnly, Category = "UI State")
    bool bMainUIVisible = false;

    /** 突发事件指示器是否显示 */
    UPROPERTY(BlueprintReadOnly, Category = "UI State|Emergency")
    bool bShowEmergencyIndicator = false;

    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 切换 MainWidget 的显示/隐藏
     * Requirements: 2.1, 2.2
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ToggleMainUI();

    /**
     * 显示 MainWidget
     * Requirements: 2.1
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ShowMainUI();

    /**
     * 隐藏 MainWidget
     * Requirements: 2.2
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void HideMainUI();

    /**
     * 切换语义地图 Widget 的显示/隐藏 (后续阶段)
     * Requirements: 7.2
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ToggleSemanticMap();

    /**
     * 显示 Direct Control 指示器
     * @param Agent 当前控制的 Agent
     * Requirements: 3.3
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ShowDirectControlIndicator(AMACharacter* Agent);

    /**
     * 隐藏 Direct Control 指示器
     * Requirements: 3.3
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void HideDirectControlIndicator();

    /**
     * 检查 Direct Control 指示器是否可见
     * @return true 如果指示器当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsDirectControlIndicatorVisible() const;

    /**
     * 获取 SimpleMainWidget 实例 (已弃用)
     * @return SimpleMainWidget 指针，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMASimpleMainWidget* GetSimpleMainWidget() const { return SimpleMainWidget; }

    /**
     * 获取 TaskPlannerWidget 实例
     * @return TaskPlannerWidget 指针，可能为 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    UMATaskPlannerWidget* GetTaskPlannerWidget() const { return TaskPlannerWidget; }

    /**
     * 加载任务图数据到 TaskPlannerWidget
     * @param Data 任务图数据
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void LoadTaskGraph(const FMATaskGraphData& Data);

    /**
     * 检查 MainUI 是否可见
     * @return true 如果 MainWidget 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI")
    bool IsMainUIVisible() const { return bMainUIVisible; }

    //=========================================================================
    // 突发事件 UI 控制
    //=========================================================================

    /**
     * 显示突发事件指示器 (红色 "突发事件" 文字)
     * Requirements: 1.1, 1.2
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void ShowEmergencyIndicator();

    /**
     * 隐藏突发事件指示器
     * Requirements: 1.3
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void HideEmergencyIndicator();

    /**
     * 检查突发事件指示器是否可见
     * @return true 如果指示器当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Emergency")
    bool IsEmergencyIndicatorVisible() const { return bShowEmergencyIndicator; }

    /**
     * 显示突发事件详情界面
     * Requirements: 2.1
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void ShowEmergencyWidget();

    /**
     * 隐藏突发事件详情界面
     * Requirements: 2.4
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void HideEmergencyWidget();

    /**
     * 切换突发事件详情界面的显示/隐藏
     * Requirements: 2.1, 2.4
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void ToggleEmergencyWidget();

    /**
     * 检查突发事件详情界面是否可见
     * @return true 如果详情界面当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Emergency")
    bool IsEmergencyWidgetVisible() const;

    /**
     * 更新突发事件相机源
     * @param Camera 相机传感器组件，nullptr 表示清除相机源
     * Requirements: 2.5, 4.4
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void UpdateEmergencyCameraSource(UMACameraSensorComponent* Camera);

    //=========================================================================
    // Modify 模式 UI 控制
    //=========================================================================

    /**
     * 显示 ModifyWidget
     * Requirements: 3.1
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Modify")
    void ShowModifyWidget();

    /**
     * 隐藏 ModifyWidget
     * Requirements: 3.4
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Modify")
    void HideModifyWidget();

    /**
     * 检查 ModifyWidget 是否可见
     * @return true 如果 ModifyWidget 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Modify")
    bool IsModifyWidgetVisible() const;

protected:
    //=========================================================================
    // AHUD 重写
    //=========================================================================

    virtual void BeginPlay() override;
    virtual void DrawHUD() override;

    //=========================================================================
    // 事件回调
    //=========================================================================

    /**
     * PlayerController 的 MainUI 切换事件回调
     * @param bVisible 是否显示
     * Requirements: 2.1, 2.2
     */
    UFUNCTION()
    void OnMainUIToggled(bool bVisible);

    /**
     * PlayerController 的重新聚焦事件回调
     * Requirements: 2.5
     */
    UFUNCTION()
    void OnRefocusMainUI();

    /**
     * EmergencyManager 状态变化回调
     * @param bIsActive 新的事件状态
     * Requirements: 1.1, 1.2, 1.3
     */
    UFUNCTION()
    void OnEmergencyStateChanged(bool bIsActive);

private:
    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 创建所有 Widget 实例
     * Requirements: 7.1
     */
    void CreateWidgets();

    /**
     * 绑定 PlayerController 事件
     */
    void BindControllerEvents();

    /**
     * 绑定 EmergencyManager 事件
     * Requirements: 1.1, 1.2, 1.3
     */
    void BindEmergencyManagerEvents();

    /**
     * 获取拥有此 HUD 的 MAPlayerController
     * @return MAPlayerController 指针，可能为 nullptr
     */
    AMAPlayerController* GetMAPlayerController() const;

    /**
     * SimpleMainWidget 指令提交回调
     * @param Command 用户输入的指令
     */
    UFUNCTION()
    void OnSimpleCommandSubmitted(const FString& Command);

    /**
     * CommSubsystem 响应回调
     * @param Response 规划器响应
     */
    UFUNCTION()
    void OnPlannerResponse(const FMAPlannerResponse& Response);

    /**
     * ModifyWidget 确认修改回调
     * @param Actor 选中的 Actor
     * @param LabelText 文本框内容
     * Requirements: 5.1, 5.2
     */
    UFUNCTION()
    void OnModifyConfirmed(AActor* Actor, const FString& LabelText);

    /**
     * PlayerController 的 Actor 选中回调
     * @param SelectedActor 选中的 Actor
     * Requirements: 4.1
     */
    UFUNCTION()
    void OnModifyActorSelected(AActor* SelectedActor);
};
