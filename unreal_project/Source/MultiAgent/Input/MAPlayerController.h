// MAPlayerController.h
// 玩家控制器 - 使用 Enhanced Input System
// 支持星际争霸风格的框选和编组
// 支持 Modify 模式的场景标注

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "../Core/Manager/MACommandManager.h"
#include "MAPlayerController.generated.h"

class UMAEditModeManager;
class AMAPointOfInterest;

struct FInputActionValue;

// 鼠标左键模式
UENUM(BlueprintType)
enum class EMAMouseMode : uint8
{
    Select      UMETA(DisplayName = "Select"),      // 框选 Agent + 视角旋转
    Deployment  UMETA(DisplayName = "Deployment"),  // 部署模式：拖拽框选区域放置 Agent
    Modify      UMETA(DisplayName = "Modify"),      // 修改模式：点击 Actor 查看/编辑标签
    Edit        UMETA(DisplayName = "Edit")         // 编辑模式：模拟任务执行中的动态变化
};

// 待部署的 Agent 配置
USTRUCT(BlueprintType)
struct FMAPendingDeployment
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString AgentType;

    UPROPERTY(BlueprintReadWrite)
    int32 Count = 0;

    FMAPendingDeployment() {}
    FMAPendingDeployment(const FString& InType, int32 InCount) : AgentType(InType), Count(InCount) {}
};

UCLASS()
class MULTIAGENT_API AMAPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMAPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

    // ========== Input Handlers ==========
    void OnLeftClick(const FInputActionValue& Value);
    void OnLeftClickReleased(const FInputActionValue& Value);
    void OnRightClick(const FInputActionValue& Value);
    void OnPickup(const FInputActionValue& Value);
    void OnDrop(const FInputActionValue& Value);
    void OnSpawnPickupItem(const FInputActionValue& Value);
    void OnSpawnQuadruped(const FInputActionValue& Value);
    void OnPrintAgentInfo(const FInputActionValue& Value);
    void OnDestroyLastAgent(const FInputActionValue& Value);
    void OnSwitchCamera(const FInputActionValue& Value);
    void OnReturnToSpectator(const FInputActionValue& Value);
    void OnStartAvoid(const FInputActionValue& Value);
    void OnStartFormation(const FInputActionValue& Value);
    void OnTakePhoto(const FInputActionValue& Value);
    void OnToggleRecording(const FInputActionValue& Value);
    void OnToggleTCPStream(const FInputActionValue& Value);
    
    // ========== 编组快捷键 (Ctrl+1~5 / 1~5) ==========
    void OnControlGroup1(const FInputActionValue& Value);
    void OnControlGroup2(const FInputActionValue& Value);
    void OnControlGroup3(const FInputActionValue& Value);
    void OnControlGroup4(const FInputActionValue& Value);
    void OnControlGroup5(const FInputActionValue& Value);
    
    // ========== 通用命令处理 ==========
    void OnStartPatrol(const FInputActionValue& Value);
    void OnStartCharge(const FInputActionValue& Value);
    void OnStopIdle(const FInputActionValue& Value);
    void OnStartCoverage(const FInputActionValue& Value);
    void OnStartFollow(const FInputActionValue& Value);
    
    // 创建/解散 Squad 快捷键
    void OnCreateSquad(const FInputActionValue& Value);
    void OnDisbandSquad(const FInputActionValue& Value);

    // 切换鼠标模式
    void OnToggleMouseMode(const FInputActionValue& Value);
    
    // 右键视角旋转
    void OnRightClickPressed(const FInputActionValue& Value);
    void OnRightClickReleased(const FInputActionValue& Value);
    
    // 中键导航
    void OnMiddleClick(const FInputActionValue& Value);

    // ========== 部署模式 ==========
    
    // 部署模式下的左键点击（开始拖拽）
    void OnDeploymentLeftClick();
    
    // 部署模式下的左键释放（完成框选放置）
    void OnDeploymentLeftClickReleased();
    
    // 将屏幕框选区域投影到世界坐标，返回生成点
    TArray<FVector> ProjectSelectionBoxToWorld(FVector2D Start, FVector2D End, int32 Count);
    
    // 投影到地面
    FVector ProjectToGround(FVector WorldLocation);
    
    // 切换主 UI 显示/隐藏 (Z 键)
    void OnToggleMainUI(const FInputActionValue& Value);

    // ========== 突发事件系统 ==========
    // 触发/结束突发事件 ("-" 键)
    void OnTriggerEmergency(const FInputActionValue& Value);
    
    // 切换突发事件详情界面 ("X" 键)
    void OnToggleEmergencyUI(const FInputActionValue& Value);

    // 跳跃 (空格键)
    void OnJumpPressed(const FInputActionValue& Value);

    // Viewport 录制 (F9 键)
    void OnToggleViewportRecording(const FInputActionValue& Value);

    // 获取鼠标点击位置
    bool GetMouseHitLocation(FVector& OutLocation);
    
    // 获取当前选中的相机
    class UMACameraSensorComponent* GetCurrentCamera();

    // ========== 框选 ==========
    void DrawSelectionBox();

public:
    // ========== 鼠标模式 ==========
    UPROPERTY(BlueprintReadOnly, Category = "Input")
    EMAMouseMode CurrentMouseMode = EMAMouseMode::Select;
    
    // 获取模式名称
    UFUNCTION(BlueprintCallable, Category = "Input")
    static FString MouseModeToString(EMAMouseMode Mode);

    // ========== 部署背包系统 ==========
    
    /** 添加待部署单位到背包 */
    UFUNCTION(BlueprintCallable, Category = "Deployment")
    void AddToDeploymentQueue(const FString& AgentType, int32 Count = 1);
    
    /** 从背包移除待部署单位 */
    UFUNCTION(BlueprintCallable, Category = "Deployment")
    void RemoveFromDeploymentQueue(const FString& AgentType, int32 Count = 1);
    
    /** 清空部署背包 */
    UFUNCTION(BlueprintCallable, Category = "Deployment")
    void ClearDeploymentQueue();
    
    /** 获取部署背包内容 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    TArray<FMAPendingDeployment> GetDeploymentQueue() const { return DeploymentQueue; }
    
    /** 获取部署背包总数 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    int32 GetDeploymentQueueCount() const;
    
    /** 背包是否有待部署单位 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    bool HasPendingDeployments() const { return DeploymentQueue.Num() > 0 && GetDeploymentQueueCount() > 0; }

    // ========== 部署模式 ==========
    
    /** 进入部署模式（使用背包中的单位） */
    UFUNCTION(BlueprintCallable, Category = "Deployment")
    void EnterDeploymentMode();
    
    /** 进入部署模式（指定单位列表，会添加到背包） */
    UFUNCTION(BlueprintCallable, Category = "Deployment")
    void EnterDeploymentModeWithUnits(const TArray<FMAPendingDeployment>& Deployments);
    
    /** 退出部署模式（保留未部署的单位在背包中） */
    UFUNCTION(BlueprintCallable, Category = "Deployment")
    void ExitDeploymentMode();
    
    /** 是否在部署模式 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    bool IsInDeploymentMode() const { return CurrentMouseMode == EMAMouseMode::Deployment; }
    
    /** 获取当前正在部署的类型 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    FString GetCurrentDeployingType() const;
    
    /** 获取当前正在部署的数量 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    int32 GetCurrentDeployingCount() const;
    
    /** 获取已部署数量（本次部署会话） */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    int32 GetDeployedCount() const { return DeployedCount; }

    /** 部署完成委托 */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeploymentCompleted);
    
    UPROPERTY(BlueprintAssignable, Category = "Deployment")
    FOnDeploymentCompleted OnDeploymentCompleted;
    
    /** 背包变化委托 */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeploymentQueueChanged);
    
    UPROPERTY(BlueprintAssignable, Category = "Deployment")
    FOnDeploymentQueueChanged OnDeploymentQueueChanged;

    // ========== Modify 模式 ==========

    /** Actor 选中委托 - 当 Modify 模式下选中单个 Actor 时广播 (向后兼容) */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModifyActorSelected, AActor*, SelectedActor);

    UPROPERTY(BlueprintAssignable, Category = "Modify")
    FOnModifyActorSelected OnModifyActorSelected;

    /** 多选委托 - 当 Modify 模式下选择集合变化时广播 */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnModifyActorsSelected, const TArray<AActor*>&, SelectedActors);

    UPROPERTY(BlueprintAssignable, Category = "Modify")
    FOnModifyActorsSelected OnModifyActorsSelected;

    /**
     * 清除 Modify 模式的高亮
     * 用于外部调用（如 MAHUD）清除当前高亮的 Actor
     */
    UFUNCTION(BlueprintCallable, Category = "Modify")
    void ClearModifyHighlight();

    /** 获取当前选择数量 */
    UFUNCTION(BlueprintPure, Category = "Modify")
    int32 GetSelectionCount() const { return HighlightedActors.Num(); }

    /** 是否处于多选状态 */
    UFUNCTION(BlueprintPure, Category = "Modify")
    bool IsMultiSelectActive() const { return HighlightedActors.Num() > 1; }

    /** 获取所有选中的 Actor */
    UFUNCTION(BlueprintPure, Category = "Modify")
    TArray<AActor*> GetHighlightedActors() const { return HighlightedActors; }

    /** 是否在 Modify 模式 */
    UFUNCTION(BlueprintPure, Category = "Modify")
    bool IsInModifyMode() const { return CurrentMouseMode == EMAMouseMode::Modify; }

private:
    // 初始化 Subsystem 缓存
    bool InitializeSubsystems();
    
    // 处理编组快捷键
    void HandleControlGroup(int32 GroupIndex);
    
    // 发送命令
    void SendCommand(EMACommand Command);

    // ========== 缓存的 Subsystem 引用 ==========
    UPROPERTY()
    class UMAAgentManager* AgentManager;

    UPROPERTY()
    class UMACommandManager* CommandManager;

    UPROPERTY()
    class UMASelectionManager* SelectionManager;

    UPROPERTY()
    class UMASquadManager* SquadManager;

    UPROPERTY()
    class UMAViewportManager* ViewportManager;

    UPROPERTY()
    class UMAEmergencyManager* EmergencyManager;

    UPROPERTY()
    class UMAEditModeManager* EditModeManager;

    // ========== 部署模式数据 ==========
    
    /** 部署背包：待部署的 Agent 列表（持久存储） */
    UPROPERTY()
    TArray<FMAPendingDeployment> DeploymentQueue;
    
    /** 当前正在部署的类型索引 */
    int32 CurrentDeploymentIndex = 0;
    
    /** 已部署数量（本次部署会话） */
    int32 DeployedCount = 0;
    
    /** 部署前的鼠标模式（用于退出时恢复） */
    EMAMouseMode PreviousMouseMode = EMAMouseMode::Select;
    
    /** 应用模式设置 */
    void ApplyMouseModeSettings(EMAMouseMode Mode);
    
    // ========== Modify 模式数据 ==========

    /** 多选集合 - 当前高亮的 Actor 列表 */
    UPROPERTY()
    TArray<AActor*> HighlightedActors;

    /** 设置 Actor 高亮状态（会自动查找根 Actor 并高亮整个 Actor 树） */
    void SetActorHighlight(AActor* Actor, bool bHighlight);

    /** 对单个 Actor 设置高亮（不递归，内部使用） */
    void SetSingleActorHighlight(AActor* Actor, bool bHighlight);

    /** 清除所有高亮 */
    void ClearAllHighlights();

    /** 添加 Actor 到选择集合 (Shift+Click) - toggle 行为 */
    void AddToSelection(AActor* Actor);

    /** 从选择集合移除 Actor */
    void RemoveFromSelection(AActor* Actor);

    /** 清除所有选择并选中单个 Actor */
    void ClearAndSelect(AActor* Actor);

    /** Modify 模式下的左键点击处理 */
    void OnModifyLeftClick();

    /** 进入 Modify 模式 */
    void EnterModifyMode();

    /** 退出 Modify 模式 */
    void ExitModifyMode();

    // ========== Edit 模式 ==========

    /** Edit 模式下的左键点击处理 */
    void OnEditLeftClick();

    /** 进入 Edit 模式 */
    void EnterEditMode();

    /** 退出 Edit 模式 */
    void ExitEditMode();

    // ========== 右键视角旋转 ==========
    
    /** 是否正在右键拖动旋转视角 */
    bool bIsRightMouseRotating = false;
    
    /** 右键按下时的鼠标位置 */
    FVector2D RightMouseStartPosition;
    
    /** 视角旋转灵敏度 */
    float CameraRotationSensitivity = 0.3f;
};
