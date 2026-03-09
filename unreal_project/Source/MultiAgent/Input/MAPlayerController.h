// MAPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "../Core/Interaction/Domain/MAInputTypes.h"
#include "../Core/Interaction/Domain/MAInteractionRuntimeTypes.h"
#include "../Core/Interaction/Domain/MAMouseModeState.h"
#include "../Core/Interaction/Domain/MADeploymentQueue.h"
#include "../Core/Interaction/Feedback/MAFeedback54.h"
#include "../Core/Interaction/Bootstrap/MAInteractionBootstrap.h"
#include "../Core/Interaction/Infrastructure/MAFeedback21Applier.h"
#include "../Core/Interaction/Application/MAModifySelectionState.h"
#include "../Core/Interaction/Application/MACommandInputCoordinator.h"
#include "../Core/Interaction/Application/MACameraInputCoordinator.h"
#include "../Core/Interaction/Application/MAAgentUtilityInputCoordinator.h"
#include "../Core/Interaction/Application/MADeploymentInputCoordinator.h"
#include "../Core/Interaction/Application/MAEditInputCoordinator.h"
#include "../Core/Interaction/Application/MAHUDShortcutCoordinator.h"
#include "../Core/Interaction/Application/MARTSSelectionInputCoordinator.h"
#include "../Core/Interaction/Application/MAModifyInputCoordinator.h"
#include "../Core/Interaction/Application/MAMouseModeCoordinator.h"
#include "../Core/Interaction/Application/MASquadInputCoordinator.h"
#include "MAPlayerController.generated.h"

class UMAHUDStateManager;
class AActor;
class AMACharacter;
class AMAPointOfInterest;

struct FInputActionValue;
struct FHitResult;

UCLASS()
class MULTIAGENT_API AMAPlayerController : public APlayerController
{
    GENERATED_BODY()

    friend class FMAEditInputCoordinator;
    friend class FMACameraInputCoordinator;
    friend class FMAAgentUtilityInputCoordinator;
    friend class FMACommandInputCoordinator;
    friend class FMADeploymentInputCoordinator;
    friend class FMARTSSelectionInputCoordinator;
    friend class FMAModifyInputCoordinator;
    friend class FMAMouseModeCoordinator;
    friend class FMASquadInputCoordinator;

public:
    AMAPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;
    void BindPointerActions(class UEnhancedInputComponent* EIC, class UMAInputActions* InputActions);
    void BindGameplayActions(class UEnhancedInputComponent* EIC, class UMAInputActions* InputActions);
    void BindHUDActions(class UEnhancedInputComponent* EIC, class UMAInputActions* InputActions);

    // ========== Input Handlers ==========
    void OnLeftClick(const FInputActionValue& Value);
    void OnLeftClickReleased(const FInputActionValue& Value);
    void OnPickup(const FInputActionValue& Value);
    void OnDrop(const FInputActionValue& Value);
    void OnSpawnPickupItem(const FInputActionValue& Value);
    void OnSpawnQuadruped(const FInputActionValue& Value);
    void OnPrintAgentInfo(const FInputActionValue& Value);
    void OnDestroyLastAgent(const FInputActionValue& Value);
    void OnSwitchCamera(const FInputActionValue& Value);
    void OnReturnToSpectator(const FInputActionValue& Value);
    void OnStartFormation(const FInputActionValue& Value);
    void OnTakePhoto(const FInputActionValue& Value);
    void OnToggleTCPStream(const FInputActionValue& Value);

    // 暂停/恢复技能执行 (P 键)
    void OnTogglePauseExecution(const FInputActionValue& Value);
    
    // ========== 编组快捷键 (Ctrl+1~5 / 1~5) ==========
    void OnControlGroup1(const FInputActionValue& Value);
    void OnControlGroup2(const FInputActionValue& Value);
    void OnControlGroup3(const FInputActionValue& Value);
    void OnControlGroup4(const FInputActionValue& Value);
    void OnControlGroup5(const FInputActionValue& Value);
    
    // ========== 通用命令处理 ==========
    void OnStartCharge(const FInputActionValue& Value);
    void OnStopIdle(const FInputActionValue& Value);
    void OnStartFollow(const FInputActionValue& Value);
    
    // 创建/解散 Squad 快捷键
    void OnCreateSquad(const FInputActionValue& Value);
    void OnDisbandSquad(const FInputActionValue& Value);

    // 切换鼠标模式
    void OnToggleMouseMode(const FInputActionValue& Value);
    
    // 切换 Modify 模式 (逗号键)
    void OnToggleModifyMode(const FInputActionValue& Value);

    // 右键视角旋转
    void OnRightClickPressed(const FInputActionValue& Value);
    void OnRightClickReleased(const FInputActionValue& Value);
    
    // 中键导航
    void OnMiddleClick(const FInputActionValue& Value);

    // ========== 部署模式 ==========
    
    // 跳跃 (空格键)
    void OnJumpPressed(const FInputActionValue& Value);

    // ========== HUD 状态管理输入 (UI Visual Redesign) ==========

    /** 处理检查任务图输入 (Z 键) - 调用 HUDStateManager::HandleCheckTaskInput */
    void OnCheckTask(const FInputActionValue& Value);

    /** 处理检查技能列表输入 (N 键) - 调用 HUDStateManager::HandleCheckSkillInput */
    void OnCheckSkill(const FInputActionValue& Value);


    /** 处理检查决策输入 (9 键) - 调用 HUDStateManager::HandleCheckDecisionInput */
    void OnCheckDecision(const FInputActionValue& Value);

    // ========== 右侧边栏面板切换 (Right Sidebar Panel Split) ==========

    /** 切换系统日志面板 (6 键) */
    void OnToggleSystemLogPanel(const FInputActionValue& Value);

    /** 切换预览面板 (7 键) */
    void OnTogglePreviewPanel(const FInputActionValue& Value);

    /** 切换指令输入面板 (8 键) */
    void OnToggleInstructionPanel(const FInputActionValue& Value);

    /** 切换 Agent 圆环高亮显示 ([ 键) */
    void OnToggleAgentHighlight(const FInputActionValue& Value);

    // 获取鼠标点击位置
    bool GetMouseHitLocation(FVector& OutLocation);
    
public:
    // 获取模式名称
    UFUNCTION(BlueprintCallable, Category = "Input")
    static FString MouseModeToString(EMAMouseMode Mode);

    UFUNCTION(BlueprintPure, Category = "Input")
    EMAMouseMode GetCurrentMouseMode() const { return MouseModeState.CurrentMode; }

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
    TArray<FMAPendingDeployment> GetDeploymentQueue() const { return DeploymentState.Items; }
    
    /** 获取部署背包总数 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    int32 GetDeploymentQueueCount() const;
    
    /** 背包是否有待部署单位 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    bool HasPendingDeployments() const { return DeploymentState.HasPendingDeployments(); }

    // ========== HUD 状态管理 (UI Visual Redesign) ==========

    /** 获取 HUD 状态管理器 */
    UFUNCTION(BlueprintPure, Category = "HUD")
    UMAHUDStateManager* GetHUDStateManager() const;

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
    bool IsInDeploymentMode() const { return MouseModeState.Is(EMAMouseMode::Deployment); }
    
    /** 获取当前正在部署的类型 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    FString GetCurrentDeployingType() const;
    
    /** 获取当前正在部署的数量 */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    int32 GetCurrentDeployingCount() const;
    
    /** 获取已部署数量（本次部署会话） */
    UFUNCTION(BlueprintPure, Category = "Deployment")
    int32 GetDeployedCount() const { return DeploymentState.DeployedCount; }

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
    int32 GetSelectionCount() const { return ModifySelectionState.Num(); }

    /** 是否处于多选状态 */
    UFUNCTION(BlueprintPure, Category = "Modify")
    bool IsMultiSelectActive() const { return ModifySelectionState.Num() > 1; }

    /** 获取所有选中的 Actor */
    UFUNCTION(BlueprintPure, Category = "Modify")
    TArray<AActor*> GetHighlightedActors() const { return ModifySelectionState.ToRawArray(); }

    /** 是否在 Modify 模式 */
    UFUNCTION(BlueprintPure, Category = "Modify")
    bool IsInModifyMode() const { return MouseModeState.Is(EMAMouseMode::Modify); }

private:
    void ApplyFeedback(const FMAFeedback21Batch& Feedback);
    bool RuntimeIsMouseOverPersistentUI() const;
    bool RuntimeIsMainUIVisible() const;
    bool RuntimeIsSelectionBoxActive() const;
    void RuntimeBeginSelectionBox(const FVector2D& Start);
    void RuntimeUpdateSelectionBox(const FVector2D& Current);
    void RuntimeEndSelectionBox(bool bAppendSelection);
    void RuntimeCancelSelectionBox();
    FVector2D RuntimeGetSelectionBoxStart() const;
    FVector2D RuntimeGetSelectionBoxEnd() const;
    bool RuntimeResolveCursorHit(ECollisionChannel CollisionChannel, FHitResult& OutHitResult);
    bool RuntimeResolveMouseHitLocation(FVector& OutLocation);
    TArray<FVector> RuntimeProjectSelectionBoxToWorld(const FVector2D& Start, const FVector2D& End, int32 Count);
    AMACharacter* RuntimeResolveClickedAgent();
    void RuntimeToggleSelection(AMACharacter* Agent);
    void RuntimeSelectAgent(AMACharacter* Agent);
    TArray<AMACharacter*> RuntimeGetSelectedAgents() const;
    void RuntimeSaveToControlGroup(int32 GroupIndex);
    void RuntimeSelectControlGroup(int32 GroupIndex);
    FMAFeedback54 RuntimeSendCommandToSelection(EMACommand Command);
    FString RuntimeGetCommandDisplayName(EMACommand Command) const;
    FMAFeedback54 RuntimeTogglePauseExecution();
    bool RuntimeSpawnAgentByType(const FString& AgentType, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
    FMAAgentRuntimeStats RuntimeGetAgentStats() const;
    bool RuntimeDestroyLastAgent(FString& OutAgentName);
    int32 RuntimeJumpSelectedAgents();
    void RuntimeNavigateSelectedAgentsToLocation(const FVector& TargetLocation);
    void RuntimeSwitchToNextCamera();
    void RuntimeReturnToSpectator();
    bool RuntimeTakePhotoForCurrentCamera(FString& OutSensorName);
    FMACameraStreamRuntimeResult RuntimeToggleTCPStreamForCurrentCamera();
    void RuntimeCycleFormation();
    bool RuntimeCreateSquadFromSelection(FString& OutSquadName, int32& OutMemberCount);
    int32 RuntimeDisbandSelectedSquads();
    bool RuntimeCanEnterEditMode() const;
    void RuntimeToggleEditSelection(AActor* HitActor);
    void RuntimeClearEditSelection();
    AMAPointOfInterest* RuntimeCreatePOI(const FVector& Location);
    void RuntimeDestroyAllPOIs();
    AActor* RuntimeFindHighlightRootActor(AActor* Actor) const;
    void RuntimeSetActorTreeHighlight(AActor* Actor, bool bHighlight) const;
    // ========== 部署模式数据 ==========

    FMAMouseModeState MouseModeState;
    FMADeploymentQueueState DeploymentState;

    FMAAgentUtilityInputCoordinator AgentUtilityInputCoordinator;
    FMAInteractionBootstrap InteractionBootstrap;
    FMAFeedback21Applier Feedback21Applier;
    FMACameraInputCoordinator CameraInputCoordinator;
    FMACommandInputCoordinator CommandInputCoordinator;
    FMADeploymentInputCoordinator DeploymentInputCoordinator;
    FMAEditInputCoordinator EditInputCoordinator;
    FMAHUDShortcutCoordinator HUDShortcutCoordinator;
    FMARTSSelectionInputCoordinator RTSSelectionInputCoordinator;
    FMAModifyInputCoordinator ModifyInputCoordinator;
    FMAMouseModeCoordinator MouseModeCoordinator;
    FMASquadInputCoordinator SquadInputCoordinator;
    
    // ========== Modify 模式数据 ==========

    FMAModifySelectionState ModifySelectionState;

    // ========== 右键视角旋转 ==========
    
    /** 是否正在右键拖动旋转视角 */
    bool bIsRightMouseRotating = false;
    
    /** 右键按下时的鼠标位置 */
    FVector2D RightMouseStartPosition;
    
    /** 视角旋转灵敏度 */
    float CameraRotationSensitivity = 0.3f;
};
