// MAPlayerController.cpp (重构版)
// 使用 Enhanced Input System + MACommandManager
// 支持星际争霸风格的框选和编组
// 支持 Modify 模式的场景标注

#include "MAPlayerController.h"
#include "MAInputActions.h"
#include "../Core/Manager/MAAgentManager.h"
#include "../Core/Manager/MACommandManager.h"
#include "../Core/Manager/MASquadManager.h"
#include "../Core/MASquad.h"
#include "../Core/Manager/MASelectionManager.h"
#include "../Core/Manager/MAViewportManager.h"
#include "../Core/Manager/MAEditModeManager.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Character/MAQuadrupedCharacter.h"
#include "../Agent/Character/MAUAVCharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AMAPlayerController::AMAPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AMAPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 缓存 Subsystem 引用
    if (!InitializeSubsystems())
    {
        UE_LOG(LogTemp, Error, TEXT("[PlayerController] Failed to initialize subsystems!"));
    }

    // 初始为 Select 模式，禁用视角控制
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    SetIgnoreLookInput(true);  // Select 模式禁用视角旋转

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        UMAInputActions* InputActions = UMAInputActions::Get();
        if (InputActions && InputActions->DefaultMappingContext)
        {
            Subsystem->AddMappingContext(InputActions->DefaultMappingContext, 0);
        }
    }
}

bool AMAPlayerController::InitializeSubsystems()
{
    UWorld* World = GetWorld();
    if (!World) return false;
    
    AgentManager = World->GetSubsystem<UMAAgentManager>();
    CommandManager = World->GetSubsystem<UMACommandManager>();
    SelectionManager = World->GetSubsystem<UMASelectionManager>();
    SquadManager = World->GetSubsystem<UMASquadManager>();
    ViewportManager = World->GetSubsystem<UMAViewportManager>();
    EditModeManager = World->GetSubsystem<UMAEditModeManager>();

    return AgentManager && CommandManager && SelectionManager && SquadManager && ViewportManager;
}

UMAHUDStateManager* AMAPlayerController::GetHUDStateManager() const
{
    return HUDShortcutCoordinator.ResolveHUDStateManager(this);
}

void AMAPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UMAInputActions* InputActions = UMAInputActions::Get();
    if (!InputActions) return;

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        BindPointerActions(EIC, InputActions);
        BindGameplayActions(EIC, InputActions);
        BindHUDActions(EIC, InputActions);
    }
}

void AMAPlayerController::BindPointerActions(UEnhancedInputComponent* EIC, UMAInputActions* InputActions)
{
    EIC->BindAction(InputActions->IA_LeftClick, ETriggerEvent::Started, this, &AMAPlayerController::OnLeftClick);
    EIC->BindAction(InputActions->IA_LeftClick, ETriggerEvent::Completed, this, &AMAPlayerController::OnLeftClickReleased);
    EIC->BindAction(InputActions->IA_RightClick, ETriggerEvent::Started, this, &AMAPlayerController::OnRightClickPressed);
    EIC->BindAction(InputActions->IA_RightClick, ETriggerEvent::Completed, this, &AMAPlayerController::OnRightClickReleased);
    EIC->BindAction(InputActions->IA_MiddleClick, ETriggerEvent::Started, this, &AMAPlayerController::OnMiddleClick);
}

void AMAPlayerController::BindGameplayActions(UEnhancedInputComponent* EIC, UMAInputActions* InputActions)
{
    EIC->BindAction(InputActions->IA_Pickup, ETriggerEvent::Started, this, &AMAPlayerController::OnPickup);
    EIC->BindAction(InputActions->IA_Drop, ETriggerEvent::Started, this, &AMAPlayerController::OnDrop);
    EIC->BindAction(InputActions->IA_SpawnItem, ETriggerEvent::Started, this, &AMAPlayerController::OnSpawnPickupItem);
    EIC->BindAction(InputActions->IA_SpawnQuadruped, ETriggerEvent::Started, this, &AMAPlayerController::OnSpawnQuadruped);
    EIC->BindAction(InputActions->IA_PrintInfo, ETriggerEvent::Started, this, &AMAPlayerController::OnPrintAgentInfo);
    EIC->BindAction(InputActions->IA_DestroyLast, ETriggerEvent::Started, this, &AMAPlayerController::OnDestroyLastAgent);
    EIC->BindAction(InputActions->IA_SwitchCamera, ETriggerEvent::Started, this, &AMAPlayerController::OnSwitchCamera);
    EIC->BindAction(InputActions->IA_ReturnSpectator, ETriggerEvent::Started, this, &AMAPlayerController::OnReturnToSpectator);
    EIC->BindAction(InputActions->IA_StartCharge, ETriggerEvent::Started, this, &AMAPlayerController::OnStartCharge);
    EIC->BindAction(InputActions->IA_StopIdle, ETriggerEvent::Started, this, &AMAPlayerController::OnStopIdle);
    EIC->BindAction(InputActions->IA_StartFollow, ETriggerEvent::Started, this, &AMAPlayerController::OnStartFollow);
    EIC->BindAction(InputActions->IA_StartFormation, ETriggerEvent::Started, this, &AMAPlayerController::OnStartFormation);
    EIC->BindAction(InputActions->IA_TakePhoto, ETriggerEvent::Started, this, &AMAPlayerController::OnTakePhoto);
    EIC->BindAction(InputActions->IA_ToggleTCPStream, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleTCPStream);
    EIC->BindAction(InputActions->IA_TogglePauseExecution, ETriggerEvent::Started, this, &AMAPlayerController::OnTogglePauseExecution);
    EIC->BindAction(InputActions->IA_ControlGroup1, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup1);
    EIC->BindAction(InputActions->IA_ControlGroup2, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup2);
    EIC->BindAction(InputActions->IA_ControlGroup3, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup3);
    EIC->BindAction(InputActions->IA_ControlGroup4, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup4);
    EIC->BindAction(InputActions->IA_ControlGroup5, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup5);
    EIC->BindAction(InputActions->IA_CreateSquad, ETriggerEvent::Started, this, &AMAPlayerController::OnCreateSquad);
    EIC->BindAction(InputActions->IA_DisbandSquad, ETriggerEvent::Started, this, &AMAPlayerController::OnDisbandSquad);
    EIC->BindAction(InputActions->IA_ToggleMouseMode, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleMouseMode);
    EIC->BindAction(InputActions->IA_ToggleModifyMode, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleModifyMode);
    EIC->BindAction(InputActions->IA_Jump, ETriggerEvent::Started, this, &AMAPlayerController::OnJumpPressed);
}

void AMAPlayerController::BindHUDActions(UEnhancedInputComponent* EIC, UMAInputActions* InputActions)
{
    EIC->BindAction(InputActions->IA_CheckTask, ETriggerEvent::Started, this, &AMAPlayerController::OnCheckTask);
    EIC->BindAction(InputActions->IA_CheckSkill, ETriggerEvent::Started, this, &AMAPlayerController::OnCheckSkill);
    EIC->BindAction(InputActions->IA_CheckDecision, ETriggerEvent::Started, this, &AMAPlayerController::OnCheckDecision);
    EIC->BindAction(InputActions->IA_ToggleSystemLogPanel, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleSystemLogPanel);
    EIC->BindAction(InputActions->IA_TogglePreviewPanel, ETriggerEvent::Started, this, &AMAPlayerController::OnTogglePreviewPanel);
    EIC->BindAction(InputActions->IA_ToggleInstructionPanel, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleInstructionPanel);
    EIC->BindAction(InputActions->IA_ToggleAgentHighlight, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleAgentHighlight);
}

// ========== 辅助方法 ==========

bool AMAPlayerController::GetMouseHitLocation(FVector& OutLocation)
{
    FHitResult HitResult;
    if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
    {
        OutLocation = HitResult.Location;
        return true;
    }
    return false;
}

// ========== 框选和选择 ==========

void AMAPlayerController::OnLeftClick(const FInputActionValue& Value)
{
    // UMG 点击和 Enhanced Input 独立分发，这里只路由场景侧输入。

    if (CurrentMouseMode == EMAMouseMode::Deployment)
    {
        DeploymentInputCoordinator.HandleLeftClickStarted(this);
        return;
    }
    
    if (CurrentMouseMode == EMAMouseMode::Modify)
    {
        ModifyInputCoordinator.HandleLeftClick(this);
        return;
    }

    if (CurrentMouseMode == EMAMouseMode::Edit)
    {
        EditInputCoordinator.HandleLeftClick(this);
        return;
    }

    RTSSelectionInputCoordinator.HandleLeftClickStarted(this);
}

void AMAPlayerController::OnLeftClickReleased(const FInputActionValue& Value)
{
    if (CurrentMouseMode == EMAMouseMode::Deployment)
    {
        DeploymentInputCoordinator.HandleLeftClickReleased(this);
        return;
    }
    
    RTSSelectionInputCoordinator.HandleLeftClickReleased(this);
}

void AMAPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    RTSSelectionInputCoordinator.Tick(this);
    CameraInputCoordinator.Tick(this);
}

void AMAPlayerController::OnRightClickPressed(const FInputActionValue& Value)
{
    CameraInputCoordinator.HandleRightClickPressed(this);
}

void AMAPlayerController::OnRightClickReleased(const FInputActionValue& Value)
{
    CameraInputCoordinator.HandleRightClickReleased(this);
}

void AMAPlayerController::OnMiddleClick(const FInputActionValue& Value)
{
    RTSSelectionInputCoordinator.HandleMiddleClickNavigate(this);
}

// ========== 拾取/放下 ==========

void AMAPlayerController::OnPickup(const FInputActionValue& Value)
{
    AgentUtilityInputCoordinator.HandlePickup();
}

void AMAPlayerController::OnDrop(const FInputActionValue& Value)
{
    AgentUtilityInputCoordinator.HandleDrop();
}

// ========== 生成 ==========

void AMAPlayerController::OnSpawnPickupItem(const FInputActionValue& Value)
{
    AgentUtilityInputCoordinator.HandleSpawnPickupItem();
}

void AMAPlayerController::OnSpawnQuadruped(const FInputActionValue& Value)
{
    AgentUtilityInputCoordinator.HandleSpawnQuadruped();
}

// ========== 调试 ==========

void AMAPlayerController::OnPrintAgentInfo(const FInputActionValue& Value)
{
    AgentUtilityInputCoordinator.HandlePrintAgentInfo(this);
}

void AMAPlayerController::OnDestroyLastAgent(const FInputActionValue& Value)
{
    AgentUtilityInputCoordinator.HandleDestroyLastAgent(this);
}

// ========== 视角切换 ==========

void AMAPlayerController::OnSwitchCamera(const FInputActionValue& Value)
{
    CameraInputCoordinator.SwitchToNextCamera(this);
}

void AMAPlayerController::OnReturnToSpectator(const FInputActionValue& Value)
{
    CameraInputCoordinator.ReturnToSpectator(this);
}

void AMAPlayerController::OnStartCharge(const FInputActionValue& Value)
{
    CommandInputCoordinator.SendCommandToSelection(SelectionManager, CommandManager, EMACommand::Charge);
}

void AMAPlayerController::OnStopIdle(const FInputActionValue& Value)
{
    CommandInputCoordinator.SendCommandToSelection(SelectionManager, CommandManager, EMACommand::Idle);
}

void AMAPlayerController::OnStartFollow(const FInputActionValue& Value)
{
    CommandInputCoordinator.SendCommandToSelection(SelectionManager, CommandManager, EMACommand::Follow);
}

void AMAPlayerController::OnStartFormation(const FInputActionValue& Value)
{
    SquadInputCoordinator.CycleFormation(this);
}

// ========== 相机操作 ==========

void AMAPlayerController::OnTakePhoto(const FInputActionValue& Value)
{
    CameraInputCoordinator.TakePhoto(this);
}

void AMAPlayerController::OnToggleTCPStream(const FInputActionValue& Value)
{
    CameraInputCoordinator.ToggleTCPStream(this);
}

// ========== 暂停/恢复技能执行 ==========

void AMAPlayerController::OnTogglePauseExecution(const FInputActionValue& Value)
{
    CommandInputCoordinator.TogglePauseExecution(CommandManager);
}

void AMAPlayerController::OnControlGroup1(const FInputActionValue& Value) { SquadInputCoordinator.HandleControlGroup(this, 1); }
void AMAPlayerController::OnControlGroup2(const FInputActionValue& Value) { SquadInputCoordinator.HandleControlGroup(this, 2); }
void AMAPlayerController::OnControlGroup3(const FInputActionValue& Value) { SquadInputCoordinator.HandleControlGroup(this, 3); }
void AMAPlayerController::OnControlGroup4(const FInputActionValue& Value) { SquadInputCoordinator.HandleControlGroup(this, 4); }
void AMAPlayerController::OnControlGroup5(const FInputActionValue& Value) { SquadInputCoordinator.HandleControlGroup(this, 5); }

void AMAPlayerController::OnCreateSquad(const FInputActionValue& Value)
{
    SquadInputCoordinator.CreateSquad(this);
}

void AMAPlayerController::OnDisbandSquad(const FInputActionValue& Value)
{
    SquadInputCoordinator.DisbandSquad(this);
}

// ========== 鼠标模式切换 ==========

void AMAPlayerController::OnToggleMouseMode(const FInputActionValue& Value)
{
    const EMAMouseMode NewMode = CurrentMouseMode == EMAMouseMode::Edit
        ? EMAMouseMode::Select
        : EMAMouseMode::Edit;

    if (!MouseModeCoordinator.TransitionToMode(this, NewMode))
    {
        return;
    }

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
        MouseModeCoordinator.BuildModeStatusText(CurrentMouseMode));

    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Mouse mode: %s"), *MouseModeToString(CurrentMouseMode));
}

// ========== Modify 模式切换 (逗号键) ==========

void AMAPlayerController::OnToggleModifyMode(const FInputActionValue& Value)
{
    const EMAMouseMode NewMode = CurrentMouseMode == EMAMouseMode::Modify
        ? EMAMouseMode::Select
        : EMAMouseMode::Modify;

    if (!MouseModeCoordinator.TransitionToMode(this, NewMode))
    {
        return;
    }

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
        MouseModeCoordinator.BuildModeStatusText(CurrentMouseMode));

    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Mouse mode: %s"), *MouseModeToString(CurrentMouseMode));
}

FString AMAPlayerController::MouseModeToString(EMAMouseMode Mode)
{
    switch (Mode)
    {
        case EMAMouseMode::Select: return TEXT("Select");
        case EMAMouseMode::Deployment: return TEXT("Deployment");
        case EMAMouseMode::Modify: return TEXT("Modify");
        case EMAMouseMode::Edit: return TEXT("Edit");
        default: return TEXT("Unknown");
    }
}

// ========== 部署背包系统 ==========

void AMAPlayerController::AddToDeploymentQueue(const FString& AgentType, int32 Count)
{
    DeploymentInputCoordinator.AddToQueue(this, AgentType, Count);
}

void AMAPlayerController::RemoveFromDeploymentQueue(const FString& AgentType, int32 Count)
{
    DeploymentInputCoordinator.RemoveFromQueue(this, AgentType, Count);
}

void AMAPlayerController::ClearDeploymentQueue()
{
    DeploymentInputCoordinator.ClearQueue(this);
}

int32 AMAPlayerController::GetDeploymentQueueCount() const
{
    return DeploymentInputCoordinator.GetQueueCount(this);
}

FString AMAPlayerController::GetCurrentDeployingType() const
{
    return DeploymentInputCoordinator.GetCurrentDeployingType(this);
}

int32 AMAPlayerController::GetCurrentDeployingCount() const
{
    return DeploymentInputCoordinator.GetCurrentDeployingCount(this);
}

// ========== 部署模式 ==========

void AMAPlayerController::EnterDeploymentMode()
{
    DeploymentInputCoordinator.EnterMode(this);
}

void AMAPlayerController::EnterDeploymentModeWithUnits(const TArray<FMAPendingDeployment>& Deployments)
{
    DeploymentInputCoordinator.EnterModeWithUnits(this, Deployments);
}

void AMAPlayerController::ExitDeploymentMode()
{
    DeploymentInputCoordinator.ExitMode(this);
}

void AMAPlayerController::OnCheckTask(const FInputActionValue& Value)
{
    HUDShortcutCoordinator.HandleCheckTask(this);
}

void AMAPlayerController::OnCheckSkill(const FInputActionValue& Value)
{
    HUDShortcutCoordinator.HandleCheckSkill(this);
}

void AMAPlayerController::OnCheckDecision(const FInputActionValue& Value)
{
    HUDShortcutCoordinator.HandleCheckDecision(this);
}

// ========== 右侧边栏面板切换 (Right Sidebar Panel Split) ==========

void AMAPlayerController::OnToggleSystemLogPanel(const FInputActionValue& Value)
{
    HUDShortcutCoordinator.ToggleSystemLogPanel(this);
}

void AMAPlayerController::OnTogglePreviewPanel(const FInputActionValue& Value)
{
    HUDShortcutCoordinator.TogglePreviewPanel(this);
}

void AMAPlayerController::OnToggleInstructionPanel(const FInputActionValue& Value)
{
    HUDShortcutCoordinator.ToggleInstructionPanel(this);
}

void AMAPlayerController::OnToggleAgentHighlight(const FInputActionValue& Value)
{
    const bool bVisible = HUDShortcutCoordinator.ToggleAgentHighlight(this);
    GEngine->AddOnScreenDebugMessage(-1, 2.f, bVisible ? FColor::Green : FColor::Yellow,
        FString::Printf(TEXT("Agent Highlight: %s"), bVisible ? TEXT("ON") : TEXT("OFF")));
}

void AMAPlayerController::OnJumpPressed(const FInputActionValue& Value)
{
    AgentUtilityInputCoordinator.HandleJumpSelection(this);
}

void AMAPlayerController::ClearModifyHighlight()
{
    ModifyInputCoordinator.ClearHighlight(this);
}
