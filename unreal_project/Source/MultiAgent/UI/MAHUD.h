// MAHUD.h
// HUD 管理器 - 管理所有 UI Widget 的创建、显示和隐藏
// 继承自 MASelectionHUD 以保留框选绘制功能

#pragma once

#include "CoreMinimal.h"
#include "MASelectionHUD.h"
#include "MAHUD.generated.h"

class UMASimpleMainWidget;
class UMATaskPlannerWidget;
class UMADirectControlIndicator;
class UMAEmergencyWidget;
class UMAModifyWidget;
class UMAEditWidget;
class UMASceneListWidget;
class AMAPlayerController;
class AMACharacter;
class UMAEmergencyManager;
class UMAEditModeManager;
class UMACameraSensorComponent;
class UMASceneGraphManager;
class AMAPointOfInterest;
struct FMAPlannerResponse;
struct FMATaskGraphData;
struct FMASceneGraphNode;

/**
 * HUD 管理器
 * 
 * 职责:
 * - 创建和管理所有 UI Widget (MainWidget, SemanticMap, GanttChart 等)
 * - 响应 PlayerController 的 UI 切换事件
 * - 管理 Widget 的显示/隐藏状态
 * - 处理输入框焦点重置
 * 
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

    /** Edit 模式编辑面板实例 */
    UPROPERTY(BlueprintReadOnly, Category = "UI Instances")
    UMAEditWidget* EditWidget;

    /** 场景列表面板实例 (Edit 模式左侧 Goal/Zone 列表) */
    UPROPERTY(BlueprintReadOnly, Category = "UI Instances")
    UMASceneListWidget* SceneListWidget;

    //=========================================================================
    // UI 状态
    //=========================================================================

    /** MainWidget 是否可见 */
    UPROPERTY(BlueprintReadOnly, Category = "UI State")
    bool bMainUIVisible = false;

    /** 突发事件指示器是否显示 */
    UPROPERTY(BlueprintReadOnly, Category = "UI State|Emergency")
    bool bShowEmergencyIndicator = false;

    /** 是否正在显示场景标签 */
    UPROPERTY(BlueprintReadOnly, Category = "UI State|SceneLabel")
    bool bShowingSceneLabels = false;

    /** 缓存的场景节点数据 */
    UPROPERTY(BlueprintReadOnly, Category = "UI State|SceneLabel")
    TArray<FMASceneGraphNode> CachedSceneNodes;

    //=========================================================================
    // 公共接口
    //=========================================================================

    /**
     * 切换 MainWidget 的显示/隐藏
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ToggleMainUI();

    /**
     * 显示 MainWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ShowMainUI();

    /**
     * 隐藏 MainWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void HideMainUI();

    /**
     * 切换语义地图 Widget 的显示/隐藏 (后续阶段)
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ToggleSemanticMap();

    /**
     * 显示 Direct Control 指示器
     * @param Agent 当前控制的 Agent
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control")
    void ShowDirectControlIndicator(AMACharacter* Agent);

    /**
     * 隐藏 Direct Control 指示器
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
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void ShowEmergencyIndicator();

    /**
     * 隐藏突发事件指示器
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
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void ShowEmergencyWidget();

    /**
     * 隐藏突发事件详情界面
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void HideEmergencyWidget();

    /**
     * 切换突发事件详情界面的显示/隐藏
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
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Emergency")
    void UpdateEmergencyCameraSource(UMACameraSensorComponent* Camera);

    //=========================================================================
    // Modify 模式 UI 控制
    //=========================================================================

    /**
     * 显示 ModifyWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Modify")
    void ShowModifyWidget();

    /**
     * 隐藏 ModifyWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Modify")
    void HideModifyWidget();

    /**
     * 检查 ModifyWidget 是否可见
     * @return true 如果 ModifyWidget 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Modify")
    bool IsModifyWidgetVisible() const;

    //=========================================================================
    // Edit 模式 UI 控制
    //=========================================================================

    /**
     * 显示 EditWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Edit")
    void ShowEditWidget();

    /**
     * 隐藏 EditWidget
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|Edit")
    void HideEditWidget();

    /**
     * 检查 EditWidget 是否可见
     * @return true 如果 EditWidget 当前可见
     */
    UFUNCTION(BlueprintPure, Category = "UI|Edit")
    bool IsEditWidgetVisible() const;

    /**
     * 检查鼠标是否在 EditWidget 上
     * 用于 Edit 模式下判断是否应该处理场景点击
     * @return true 如果鼠标在 EditWidget 或 SceneListWidget 上
     */
    UFUNCTION(BlueprintPure, Category = "UI|Edit")
    bool IsMouseOverEditWidget() const;

    //=========================================================================
    // 场景标签可视化
    //=========================================================================

    /**
     * 开始场景标签可视化
     * 加载场景图数据并开始显示标签
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|SceneLabel")
    void StartSceneLabelVisualization();

    /**
     * 停止场景标签可视化
     * 停止显示标签并清空缓存
     */
    UFUNCTION(BlueprintCallable, Category = "UI Control|SceneLabel")
    void StopSceneLabelVisualization();

    /**
     * 检查场景标签可视化是否激活
     * @return true 如果正在显示场景标签
     */
    UFUNCTION(BlueprintPure, Category = "UI|SceneLabel")
    bool IsSceneLabelVisualizationActive() const { return bShowingSceneLabels; }

    //=========================================================================
    // 通知系统
    //=========================================================================

    /**
     * 显示通知消息
     * @param Message 通知消息内容
     * @param bIsError 是否为错误消息 (红色)，false 为成功消息 (绿色)
     * @param bIsWarning 是否为警告消息 (黄色)，优先级低于 bIsError
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Notification")
    void ShowNotification(const FString& Message, bool bIsError = false, bool bIsWarning = false);

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
     */
    UFUNCTION()
    void OnMainUIToggled(bool bVisible);

    /**
     * PlayerController 的重新聚焦事件回调
     */
    UFUNCTION()
    void OnRefocusMainUI();

    /**
     * EmergencyManager 状态变化回调
     * @param bIsActive 新的事件状态
     */
    UFUNCTION()
    void OnEmergencyStateChanged(bool bIsActive);

private:
    //=========================================================================
    // 内部方法
    //=========================================================================

    /**
     * 创建所有 Widget 实例
     */
    void CreateWidgets();

    /**
     * 绑定 PlayerController 事件
     */
    void BindControllerEvents();

    /**
     * 绑定 EmergencyManager 事件
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
     * ModifyWidget 确认修改回调 (单选模式)
     * @param Actor 选中的 Actor
     * @param LabelText 文本框内容
     */
    UFUNCTION()
    void OnModifyConfirmed(AActor* Actor, const FString& LabelText);

    /**
     * ModifyWidget 确认修改回调 (多选模式)
     * @param Actors 选中的 Actor 数组
     * @param LabelText 文本框内容
     * @param GeneratedJson 生成的 JSON 字符串
     */
    UFUNCTION()
    void OnMultiSelectModifyConfirmed(const TArray<AActor*>& Actors, const FString& LabelText, const FString& GeneratedJson);

    /**
     * PlayerController 的 Actor 选中回调 (单选模式)
     * @param SelectedActor 选中的 Actor
     */
    UFUNCTION()
    void OnModifyActorSelected(AActor* SelectedActor);

    /**
     * PlayerController 的 Actor 选中回调 (多选模式)
     * @param SelectedActors 选中的 Actor 数组
     */
    UFUNCTION()
    void OnModifyActorsSelected(const TArray<AActor*>& SelectedActors);

    //=========================================================================
    // 通知系统内部
    //=========================================================================

    /** 当前通知消息 */
    FString CurrentNotificationMessage;

    /** 通知消息颜色 */
    FLinearColor CurrentNotificationColor;

    /** 是否显示通知 */
    bool bShowNotification = false;

    /** 通知显示开始时间 */
    float NotificationStartTime = 0.0f;

    /** 通知显示持续时间 (秒) */
    static constexpr float NotificationDuration = 3.0f;

    /**
     * 绘制通知消息
     */
    void DrawNotification();

    //=========================================================================
    // 场景标签可视化内部方法
    //=========================================================================

    /**
     * 加载场景图数据用于可视化
     * 从 SceneGraphManager 获取所有节点并缓存
     */
    void LoadSceneGraphForVisualization();

    /**
     * 绘制场景标签
     * 在 DrawHUD() 中调用，遍历缓存的节点并绘制绿色文本
     */
    void DrawSceneLabels();

    /**
     * 清除高亮的 Actor
     * 通过 PlayerController 清除当前高亮
     */
    void ClearHighlightedActor();

    //=========================================================================
    // Edit 模式内部方法
    //=========================================================================

    /**
     * 绑定 EditModeManager 事件
     */
    void BindEditModeManagerEvents();

    /**
     * 绑定 EditWidget 委托
     */
    void BindEditWidgetDelegates();

    /**
     * 绘制 Edit 模式指示器和 POI 坐标
     */
    void DrawEditModeIndicator();

    /**
     * EditModeManager 选择变化回调
     */
    UFUNCTION()
    void OnEditModeSelectionChanged();

    /**
     * EditModeManager 临时场景图变化回调
     */
    UFUNCTION()
    void OnTempSceneGraphChanged();

    /**
     * EditWidget 确认修改回调
     * @param Actor 选中的 Actor
     * @param JsonContent JSON 编辑内容
     */
    UFUNCTION()
    void OnEditConfirmed(AActor* Actor, const FString& JsonContent);

    /**
     * EditWidget 删除 Actor 回调
     * @param Actor 要删除的 Actor
     */
    UFUNCTION()
    void OnEditDeleteActor(AActor* Actor);

    /**
     * EditWidget 创建 Goal 回调
     * @param POI 用于创建 Goal 的 POI
     * @param Description Goal 描述
     */
    UFUNCTION()
    void OnEditCreateGoal(AMAPointOfInterest* POI, const FString& Description);

    /**
     * EditWidget 创建 Zone 回调
     * @param POIs 用于创建 Zone 的 POI 数组
     * @param Description Zone 描述
     */
    UFUNCTION()
    void OnEditCreateZone(const TArray<AMAPointOfInterest*>& POIs, const FString& Description);

    /**
     * EditWidget 添加预设 Actor 回调
     * @param POI 目标 POI
     * @param ActorType 预设 Actor 类型
     */
    UFUNCTION()
    void OnEditAddPresetActor(AMAPointOfInterest* POI, const FString& ActorType);

    /**
     * EditWidget 删除 POI 回调
     * @param POIs 要删除的 POI 数组
     */
    UFUNCTION()
    void OnEditDeletePOIs(const TArray<AMAPointOfInterest*>& POIs);

    /**
     * EditWidget 设为 Goal 回调
     * @param Actor 目标 Actor
     */
    UFUNCTION()
    void OnEditSetAsGoal(AActor* Actor);

    /**
     * EditWidget 取消 Goal 回调
     * @param Actor 目标 Actor
     */
    UFUNCTION()
    void OnEditUnsetAsGoal(AActor* Actor);

    /**
     * SceneListWidget Goal 项点击回调
     * @param GoalId Goal Node ID
     */
    UFUNCTION()
    void OnSceneListGoalClicked(const FString& GoalId);

    /**
     * SceneListWidget Zone 项点击回调
     * @param ZoneId Zone Node ID
     */
    UFUNCTION()
    void OnSceneListZoneClicked(const FString& ZoneId);
};
