// MAPlayerController.cpp (重构版)
// 使用 Enhanced Input System + MACommandManager
// 支持星际争霸风格的框选和编组
// 支持 Modify 模式的场景标注

#include "MAPlayerController.h"
#include "MAInputActions.h"
#include "../Core/Manager/MAAgentManager.h"
#include "../Core/Manager/MAViewportRecorder.h"
#include "../UI/HUD/MAHUD.h"
#include "../UI/Core/MAUIManager.h"
#include "../Core/Manager/MACommandManager.h"
#include "../Core/Manager/MASquadManager.h"
#include "../Core/MASquad.h"
#include "../Core/Manager/MASelectionManager.h"
#include "../Core/Manager/MAViewportManager.h"
#include "../Core/Manager/MAEmergencyManager.h"
#include "../Core/Manager/MAEditModeManager.h"
#include "../Environment/MAPointOfInterest.h"
#include "../UI/Core/MAHUDStateManager.h"
#include "../UI/HUD/MASelectionHUD.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Character/MAQuadrupedCharacter.h"
#include "../Agent/Character/MAUAVCharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Canvas.h"
#include "Components/PrimitiveComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

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

    // 延迟获取 HUDStateManager (等待 HUD 初始化完成)
    // HUD 在 PlayerController::BeginPlay 之后才会创建
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
        {
            if (UMAUIManager* UIManager = HUD->GetUIManager())
            {
                HUDStateManager = UIManager->GetHUDStateManager();
                if (HUDStateManager)
                {
                    UE_LOG(LogTemp, Log, TEXT("[PlayerController] HUDStateManager obtained from UIManager"));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] HUDStateManager is null in UIManager"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[PlayerController] UIManager is null in HUD"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[PlayerController] MAHUD not found, HUDStateManager will be null"));
        }
    });
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
    EmergencyManager = World->GetSubsystem<UMAEmergencyManager>();
    EditModeManager = World->GetSubsystem<UMAEditModeManager>();

    // 注意: HUDStateManager 不在这里创建
    // 它由 MAHUD->UIManager 创建和管理
    // 在 BeginPlay 中 HUD 初始化后，通过 GetHUDStateManagerFromHUD() 获取引用

    return AgentManager && CommandManager && SelectionManager && SquadManager && ViewportManager && EmergencyManager;
}

void AMAPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UMAInputActions* InputActions = UMAInputActions::Get();
    if (!InputActions) return;

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // 鼠标点击 (框选)
        EIC->BindAction(InputActions->IA_LeftClick, ETriggerEvent::Started, this, &AMAPlayerController::OnLeftClick);
        EIC->BindAction(InputActions->IA_LeftClick, ETriggerEvent::Completed, this, &AMAPlayerController::OnLeftClickReleased);
        EIC->BindAction(InputActions->IA_RightClick, ETriggerEvent::Started, this, &AMAPlayerController::OnRightClickPressed);
        EIC->BindAction(InputActions->IA_RightClick, ETriggerEvent::Completed, this, &AMAPlayerController::OnRightClickReleased);
        EIC->BindAction(InputActions->IA_MiddleClick, ETriggerEvent::Started, this, &AMAPlayerController::OnMiddleClick);

        // GAS 技能
        EIC->BindAction(InputActions->IA_Pickup, ETriggerEvent::Started, this, &AMAPlayerController::OnPickup);
        EIC->BindAction(InputActions->IA_Drop, ETriggerEvent::Started, this, &AMAPlayerController::OnDrop);

        // 生成
        EIC->BindAction(InputActions->IA_SpawnItem, ETriggerEvent::Started, this, &AMAPlayerController::OnSpawnPickupItem);
        EIC->BindAction(InputActions->IA_SpawnQuadruped, ETriggerEvent::Started, this, &AMAPlayerController::OnSpawnQuadruped);

        // 调试
        EIC->BindAction(InputActions->IA_PrintInfo, ETriggerEvent::Started, this, &AMAPlayerController::OnPrintAgentInfo);
        EIC->BindAction(InputActions->IA_DestroyLast, ETriggerEvent::Started, this, &AMAPlayerController::OnDestroyLastAgent);

        // 视角切换
        EIC->BindAction(InputActions->IA_SwitchCamera, ETriggerEvent::Started, this, &AMAPlayerController::OnSwitchCamera);
        EIC->BindAction(InputActions->IA_ReturnSpectator, ETriggerEvent::Started, this, &AMAPlayerController::OnReturnToSpectator);

        // 命令 (通过 CommandManager)
        EIC->BindAction(InputActions->IA_StartPatrol, ETriggerEvent::Started, this, &AMAPlayerController::OnStartPatrol);
        EIC->BindAction(InputActions->IA_StartCharge, ETriggerEvent::Started, this, &AMAPlayerController::OnStartCharge);
        EIC->BindAction(InputActions->IA_StopIdle, ETriggerEvent::Started, this, &AMAPlayerController::OnStopIdle);
        EIC->BindAction(InputActions->IA_StartCoverage, ETriggerEvent::Started, this, &AMAPlayerController::OnStartCoverage);
        EIC->BindAction(InputActions->IA_StartFollow, ETriggerEvent::Started, this, &AMAPlayerController::OnStartFollow);
        EIC->BindAction(InputActions->IA_StartAvoid, ETriggerEvent::Started, this, &AMAPlayerController::OnStartAvoid);
        EIC->BindAction(InputActions->IA_StartFormation, ETriggerEvent::Started, this, &AMAPlayerController::OnStartFormation);

        // 相机
        EIC->BindAction(InputActions->IA_TakePhoto, ETriggerEvent::Started, this, &AMAPlayerController::OnTakePhoto);
        EIC->BindAction(InputActions->IA_ToggleRecording, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleRecording);
        EIC->BindAction(InputActions->IA_ToggleTCPStream, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleTCPStream);

        // 编组快捷键 (星际争霸风格: 1~5 选择, Ctrl+1~5 保存)
        EIC->BindAction(InputActions->IA_ControlGroup1, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup1);
        EIC->BindAction(InputActions->IA_ControlGroup2, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup2);
        EIC->BindAction(InputActions->IA_ControlGroup3, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup3);
        EIC->BindAction(InputActions->IA_ControlGroup4, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup4);
        EIC->BindAction(InputActions->IA_ControlGroup5, ETriggerEvent::Started, this, &AMAPlayerController::OnControlGroup5);

        // 创建/解散 Squad
        EIC->BindAction(InputActions->IA_CreateSquad, ETriggerEvent::Started, this, &AMAPlayerController::OnCreateSquad);
        EIC->BindAction(InputActions->IA_DisbandSquad, ETriggerEvent::Started, this, &AMAPlayerController::OnDisbandSquad);

        // 鼠标模式切换
        EIC->BindAction(InputActions->IA_ToggleMouseMode, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleMouseMode);
        EIC->BindAction(InputActions->IA_ToggleModifyMode, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleModifyMode);

        // UI 切换 (Z 键)
        EIC->BindAction(InputActions->IA_ToggleMainUI, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleMainUI);

        // 技能分配查看器切换 (N 键)
        EIC->BindAction(InputActions->IA_ToggleSkillAllocationViewer, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleSkillAllocationViewer);

        // 突发事件系统
        EIC->BindAction(InputActions->IA_TriggerEmergency, ETriggerEvent::Started, this, &AMAPlayerController::OnTriggerEmergency);
        EIC->BindAction(InputActions->IA_ToggleEmergencyUI, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleEmergencyUI);

        // 跳跃 (空格键)
        EIC->BindAction(InputActions->IA_Jump, ETriggerEvent::Started, this, &AMAPlayerController::OnJumpPressed);

        // Viewport 录制 (F9 键)
        EIC->BindAction(InputActions->IA_ToggleViewportRecording, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleViewportRecording);

        // ========== HUD 状态管理输入 (UI Visual Redesign) ==========
        // Requirements: 10.4
        // 绑定到 HUDStateManager 的输入处理方法
        EIC->BindAction(InputActions->IA_CheckTask, ETriggerEvent::Started, this, &AMAPlayerController::OnCheckTask);
        EIC->BindAction(InputActions->IA_CheckSkill, ETriggerEvent::Started, this, &AMAPlayerController::OnCheckSkill);
        EIC->BindAction(InputActions->IA_CheckUnexpected, ETriggerEvent::Started, this, &AMAPlayerController::OnCheckUnexpected);
    }
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
    // 注意: 不再在这里检测 UI，因为 UMG 按钮的 OnClicked 事件是独立于 Enhanced Input 的
    // Modify 模式也没有 UI 检测，按钮点击仍然正常工作
    // Edit 模式采用相同的方式：让 OnEditLeftClick 处理场景交互，UMG 自己处理按钮点击

    if (CurrentMouseMode == EMAMouseMode::Deployment)
    {
        // 部署模式：开始框选部署区域
        OnDeploymentLeftClick();
        return;
    }
    
    if (CurrentMouseMode == EMAMouseMode::Modify)
    {
        // Modify 模式：点击选择 Actor
        OnModifyLeftClick();
        return;
    }

    if (CurrentMouseMode == EMAMouseMode::Edit)
    {
        // Edit 模式：点击创建 POI 或选择对象
        OnEditLeftClick();
        return;
    }

    // Select 模式：开始框选
    if (SelectionManager)
    {
        float MouseX, MouseY;
        if (GetMousePosition(MouseX, MouseY))
        {
            SelectionManager->BeginBoxSelect(FVector2D(MouseX, MouseY));
        }
    }
}

void AMAPlayerController::OnLeftClickReleased(const FInputActionValue& Value)
{
    if (CurrentMouseMode == EMAMouseMode::Deployment)
    {
        // 部署模式：完成框选放置
        OnDeploymentLeftClickReleased();
        return;
    }
    
    if (!SelectionManager || !SelectionManager->IsBoxSelecting()) return;

    // 更新终点
    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionManager->UpdateBoxSelect(FVector2D(MouseX, MouseY));
    }

    // 计算框选大小
    FVector2D Start = SelectionManager->GetBoxSelectStart();
    FVector2D End = SelectionManager->GetBoxSelectEnd();
    float BoxWidth = FMath::Abs(End.X - Start.X);
    float BoxHeight = FMath::Abs(End.Y - Start.Y);

    bool bCtrlPressed = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);

    if (BoxWidth < 10.f && BoxHeight < 10.f)
    {
        // 点击选择：检查是否点击了 Agent
        SelectionManager->CancelBoxSelect();
        
        FHitResult HitResult;
        if (GetHitResultUnderCursor(ECC_Pawn, false, HitResult))
        {
            if (AMACharacter* Agent = Cast<AMACharacter>(HitResult.GetActor()))
            {
                if (bCtrlPressed)
                {
                    SelectionManager->ToggleSelection(Agent);
                }
                else
                {
                    SelectionManager->SelectAgent(Agent);
                }
                return;
            }
        }
        
        // 点击空地：不清除选择，保持当前选中状态
        // 如果需要清除选择，可以按 Esc 或点击已选中的 Agent
    }
    else
    {
        // 框选
        SelectionManager->EndBoxSelect(this, bCtrlPressed);
    }
}

void AMAPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新框选终点
    if (SelectionManager && SelectionManager->IsBoxSelecting())
    {
        float MouseX, MouseY;
        if (GetMousePosition(MouseX, MouseY))
        {
            SelectionManager->UpdateBoxSelect(FVector2D(MouseX, MouseY));
        }
    }

    // 更新 HUD 的框选状态 (MAHUD 继承自 MASelectionHUD)
    if (AMASelectionHUD* SelectionHUD = Cast<AMASelectionHUD>(GetHUD()))
    {
        if (SelectionManager)
        {
            SelectionHUD->bIsBoxSelecting = SelectionManager->IsBoxSelecting();
            SelectionHUD->BoxStart = SelectionManager->GetBoxSelectStart();
            SelectionHUD->BoxEnd = SelectionManager->GetBoxSelectEnd();
        }
    }
    
    // 右键拖动旋转视角
    if (bIsRightMouseRotating)
    {
        float MouseX, MouseY;
        if (GetMousePosition(MouseX, MouseY))
        {
            FVector2D CurrentPos(MouseX, MouseY);
            FVector2D Delta = CurrentPos - RightMouseStartPosition;
            
            // 只有拖动超过阈值才旋转（避免抖动）
            if (Delta.Size() > 2.f)
            {
                // 获取当前 Pawn 并旋转
                if (APawn* ControlledPawn = GetPawn())
                {
                    // Yaw (左右旋转) - 鼠标 X 移动
                    // Pitch (上下旋转) - 鼠标 Y 移动
                    FRotator NewRotation = ControlledPawn->GetActorRotation();
                    NewRotation.Yaw += Delta.X * CameraRotationSensitivity;
                    NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch - Delta.Y * CameraRotationSensitivity, -89.f, 89.f);
                    
                    ControlledPawn->SetActorRotation(NewRotation);
                    SetControlRotation(NewRotation);
                }
                
                // 更新起始位置，实现连续旋转
                RightMouseStartPosition = CurrentPos;
            }
        }
    }
}

void AMAPlayerController::OnRightClickPressed(const FInputActionValue& Value)
{
    // 检查 UI 是否可见
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        if (HUD->IsMainUIVisible() || HUD->IsEmergencyWidgetVisible())
        {
            return;
        }
    }
    
    // 开始右键旋转
    bIsRightMouseRotating = true;
    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY))
    {
        RightMouseStartPosition = FVector2D(MouseX, MouseY);
    }
}

void AMAPlayerController::OnRightClickReleased(const FInputActionValue& Value)
{
    // 右键只用于旋转视角，释放时结束旋转
    bIsRightMouseRotating = false;
}

void AMAPlayerController::OnMiddleClick(const FInputActionValue& Value)
{
    // 中键：导航已选中的 Agent 到点击位置
    if (!SelectionManager) return;
    
    TArray<AMACharacter*> SelectedAgents = SelectionManager->GetSelectedAgents();
    if (SelectedAgents.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("No agents selected"));
        return;
    }
    
    FVector HitLocation;
    if (!GetMouseHitLocation(HitLocation)) return;

    // 检查 UI 是否可见
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        if (HUD->IsMainUIVisible() || HUD->IsEmergencyWidgetVisible())
        {
            return;
        }
    }

    for (AMACharacter* Agent : SelectedAgents)
    {
        if (!Agent) continue;

        FVector Target = HitLocation;
        if (Agent->AgentType == EMAAgentType::UAV)
        {
            if (AMAUAVCharacter* UAV = Cast<AMAUAVCharacter>(Agent))
            {
                if (UAV->IsInAir())
                {
                    Target.Z = Agent->GetActorLocation().Z;
                }
            }
        }
        Agent->TryNavigateTo(Target);
    }
}

// ========== 拾取/放下 ==========

void AMAPlayerController::OnPickup(const FInputActionValue& Value)
{
    // Place 技能处理物品操作
}

void AMAPlayerController::OnDrop(const FInputActionValue& Value)
{
    // Place 技能处理物品操作
}

// ========== 生成 ==========

void AMAPlayerController::OnSpawnPickupItem(const FInputActionValue& Value)
{
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("SpawnItem disabled - use Place skill"));
}

void AMAPlayerController::OnSpawnQuadruped(const FInputActionValue& Value)
{
    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("SpawnQuadruped disabled - use config/agents.json"));
}

// ========== 调试 ==========

void AMAPlayerController::OnPrintAgentInfo(const FInputActionValue& Value)
{
    if (!AgentManager) return;

    int32 Total = AgentManager->GetAgentCount();
    int32 Quadrupeds = AgentManager->GetAgentsByType(EMAAgentType::Quadruped).Num();
    int32 Humanoids = AgentManager->GetAgentsByType(EMAAgentType::Humanoid).Num();
    int32 UAVs = AgentManager->GetAgentsByType(EMAAgentType::UAV).Num();

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, 
        FString::Printf(TEXT("=== Agents: %d | UAVs: %d | Quadrupeds: %d | Humanoids: %d ==="),
            Total, UAVs, Quadrupeds, Humanoids));

    for (AMACharacter* Agent : AgentManager->GetAllAgents())
    {
        if (Agent)
        {
            FString Info = FString::Printf(TEXT("  [%s] %s"), *Agent->AgentID, *Agent->AgentName);
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Info);
        }
    }
}

void AMAPlayerController::OnDestroyLastAgent(const FInputActionValue& Value)
{
    if (!AgentManager) return;

    TArray<AMACharacter*> AllAgents = AgentManager->GetAllAgents();
    if (AllAgents.Num() > 0)
    {
        AMACharacter* LastAgent = AllAgents.Last();
        FString Name = LastAgent->AgentName;
        if (AgentManager->DestroyAgent(LastAgent))
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
                FString::Printf(TEXT("Destroyed: %s"), *Name));
        }
    }
}

// ========== 视角切换 ==========

void AMAPlayerController::OnSwitchCamera(const FInputActionValue& Value)
{
    if (ViewportManager) ViewportManager->SwitchToNextCamera(this);
}

void AMAPlayerController::OnReturnToSpectator(const FInputActionValue& Value)
{
    if (ViewportManager) ViewportManager->ReturnToSpectator(this);
}

// ========== 命令 (通过 CommandManager) ==========

void AMAPlayerController::SendCommand(EMACommand Command)
{
    // 旧接口已移除
}

void AMAPlayerController::OnStartPatrol(const FInputActionValue& Value) { /* Patrol 已移除 */ }
void AMAPlayerController::OnStartCharge(const FInputActionValue& Value) { SendCommand(EMACommand::Charge); }
void AMAPlayerController::OnStopIdle(const FInputActionValue& Value) { SendCommand(EMACommand::Idle); }
void AMAPlayerController::OnStartCoverage(const FInputActionValue& Value) { /* Coverage 已移除 */ }
void AMAPlayerController::OnStartFollow(const FInputActionValue& Value) { SendCommand(EMACommand::Follow); }
void AMAPlayerController::OnStartAvoid(const FInputActionValue& Value) { /* Avoid 已移除 */ }

void AMAPlayerController::OnStartFormation(const FInputActionValue& Value)
{
    if (!SquadManager) return;

    UMASquad* Squad = SquadManager->GetOrCreateDefaultSquad();
    if (Squad) SquadManager->CycleFormation(Squad);
}

// ========== 相机操作 ==========

void AMAPlayerController::OnTakePhoto(const FInputActionValue& Value)
{
    UMACameraSensorComponent* Camera = GetCurrentCamera();
    if (Camera && Camera->TakePhoto())
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("%s: Photo saved"), *Camera->SensorName));
    }
}

void AMAPlayerController::OnToggleRecording(const FInputActionValue& Value)
{
    UMACameraSensorComponent* Camera = GetCurrentCamera();
    if (!Camera) return;

    if (Camera->bIsRecording)
    {
        Camera->StopRecording();
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString::Printf(TEXT("%s: Recording stopped"), *Camera->SensorName));
    }
    else
    {
        Camera->StartRecording(Camera->StreamFPS);
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            FString::Printf(TEXT("%s: Recording started"), *Camera->SensorName));
    }
}

void AMAPlayerController::OnToggleTCPStream(const FInputActionValue& Value)
{
    UMACameraSensorComponent* Camera = GetCurrentCamera();
    if (!Camera) return;

    // 停止其他相机的流
    if (AgentManager)
    {
        for (AMACharacter* Agent : AgentManager->GetAllAgents())
        {
            if (Agent)
            {
                if (UMACameraSensorComponent* OtherCamera = Agent->GetCameraSensor())
                {
                    if (OtherCamera != Camera && OtherCamera->bIsStreaming)
                    {
                        OtherCamera->StopTCPStream();
                    }
                }
            }
        }
    }

    if (Camera->bIsStreaming)
    {
        Camera->StopTCPStream();
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
            FString::Printf(TEXT("%s: TCP stopped"), *Camera->SensorName));
    }
    else
    {
        if (Camera->StartTCPStream(9000, Camera->StreamFPS))
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
                FString::Printf(TEXT("%s: TCP on port 9000"), *Camera->SensorName));
        }
    }
}

// ========== 辅助：获取当前相机 ==========

UMACameraSensorComponent* AMAPlayerController::GetCurrentCamera()
{
    return ViewportManager ? ViewportManager->GetCurrentCamera() : nullptr;
}

// ========== 编组快捷键 (星际争霸风格) ==========

void AMAPlayerController::HandleControlGroup(int32 GroupIndex)
{
    if (!SelectionManager) return;

    bool bCtrlPressed = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);

    if (bCtrlPressed)
    {
        // Ctrl + 数字：保存当前选择到编组
        SelectionManager->SaveToControlGroup(GroupIndex);
    }
    else
    {
        // 数字：选中编组
        SelectionManager->SelectControlGroup(GroupIndex);
    }
}

void AMAPlayerController::OnControlGroup1(const FInputActionValue& Value) { HandleControlGroup(1); }
void AMAPlayerController::OnControlGroup2(const FInputActionValue& Value) { HandleControlGroup(2); }
void AMAPlayerController::OnControlGroup3(const FInputActionValue& Value) { HandleControlGroup(3); }
void AMAPlayerController::OnControlGroup4(const FInputActionValue& Value) { HandleControlGroup(4); }
void AMAPlayerController::OnControlGroup5(const FInputActionValue& Value) { HandleControlGroup(5); }

// ========== 创建 Squad ==========

void AMAPlayerController::OnCreateSquad(const FInputActionValue& Value)
{
    // Shift+Q 由 OnDisbandSquad 处理
    if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift)) return;
    if (!SelectionManager || !SquadManager) return;

    TArray<AMACharacter*> Selected = SelectionManager->GetSelectedAgents();
    if (Selected.Num() < 2)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, 
            TEXT("Select at least 2 agents to create squad (Q)"));
        return;
    }

    // 第一个选中的作为 Leader
    UMASquad* Squad = SquadManager->CreateSquad(Selected, Selected[0]);
    
    if (Squad)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
            FString::Printf(TEXT("Created Squad '%s' with %d members"), 
                *Squad->SquadName, Squad->GetMemberCount()));
    }
}

void AMAPlayerController::OnDisbandSquad(const FInputActionValue& Value)
{
    // 只有 Shift+Q 才解散
    if (!IsInputKeyDown(EKeys::LeftShift) && !IsInputKeyDown(EKeys::RightShift)) return;
    if (!SelectionManager || !SquadManager) return;

    TArray<AMACharacter*> Selected = SelectionManager->GetSelectedAgents();
    
    // 收集选中 Agent 所属的 Squad
    TSet<UMASquad*> SquadsToDisbandSet;
    for (AMACharacter* Agent : Selected)
    {
        if (Agent && Agent->CurrentSquad)
        {
            SquadsToDisbandSet.Add(Agent->CurrentSquad);
        }
    }

    if (SquadsToDisbandSet.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, 
            TEXT("No squad to disband (Shift+Q)"));
        return;
    }

    // 解散所有相关 Squad
    int32 DisbandedCount = 0;
    for (UMASquad* Squad : SquadsToDisbandSet)
    {
        if (SquadManager->DisbandSquad(Squad))
        {
            DisbandedCount++;
        }
    }

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
        FString::Printf(TEXT("Disbanded %d squad(s)"), DisbandedCount));
}

// ========== 鼠标模式切换 ==========

void AMAPlayerController::OnToggleMouseMode(const FInputActionValue& Value)
{
    // M 键: 切换 Edit 模式
    // 如果当前在 Edit 模式，退出到 Select 模式
    // 如果当前不在 Edit 模式，进入 Edit 模式（如果在 Modify 模式，先退出）
    
    EMAMouseMode NewMode;
    
    if (CurrentMouseMode == EMAMouseMode::Edit)
    {
        // 退出 Edit 模式，返回 Select
        NewMode = EMAMouseMode::Select;
    }
    else
    {
        // 进入 Edit 模式
        // 如果当前在 Modify 模式，先退出
        if (CurrentMouseMode == EMAMouseMode::Modify)
        {
            ExitModifyMode();
        }
        NewMode = EMAMouseMode::Edit;
    }
    
    // 如果从部署模式切出，保存状态
    if (CurrentMouseMode == EMAMouseMode::Deployment && NewMode != EMAMouseMode::Deployment)
    {
        // 取消正在进行的框选
        if (SelectionManager && SelectionManager->IsBoxSelecting())
        {
            SelectionManager->CancelBoxSelect();
        }
        CurrentDeploymentIndex = 0;
    }

    // 进入新模式
    if (NewMode == EMAMouseMode::Edit)
    {
        EnterEditMode();
    }

    PreviousMouseMode = CurrentMouseMode;
    CurrentMouseMode = NewMode;
    ApplyMouseModeSettings(NewMode);

    FString ModeName = MouseModeToString(CurrentMouseMode);
    FString ExtraInfo;

    if (CurrentMouseMode == EMAMouseMode::Edit)
    {
        ExtraInfo = TEXT(" [Click to create POI, Shift+Click to select]");
    }

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
        FString::Printf(TEXT("Mode: %s%s (M to toggle Edit)"), *ModeName, *ExtraInfo));

    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Mouse mode: %s"), *ModeName);
}

// ========== Modify 模式切换 (逗号键) ==========

void AMAPlayerController::OnToggleModifyMode(const FInputActionValue& Value)
{
    // 逗号键: 切换 Modify 模式
    // 如果当前在 Modify 模式，退出到 Select 模式
    // 如果当前不在 Modify 模式，进入 Modify 模式（如果在 Edit 模式，先退出）
    
    EMAMouseMode NewMode;
    
    if (CurrentMouseMode == EMAMouseMode::Modify)
    {
        // 退出 Modify 模式，返回 Select
        NewMode = EMAMouseMode::Select;
    }
    else
    {
        // 进入 Modify 模式
        // 如果当前在 Edit 模式，先退出
        if (CurrentMouseMode == EMAMouseMode::Edit)
        {
            ExitEditMode();
        }
        NewMode = EMAMouseMode::Modify;
    }
    
    // 如果从部署模式切出，保存状态
    if (CurrentMouseMode == EMAMouseMode::Deployment && NewMode != EMAMouseMode::Deployment)
    {
        // 取消正在进行的框选
        if (SelectionManager && SelectionManager->IsBoxSelecting())
        {
            SelectionManager->CancelBoxSelect();
        }
        CurrentDeploymentIndex = 0;
    }

    // 进入新模式
    if (NewMode == EMAMouseMode::Modify)
    {
        EnterModifyMode();
    }

    PreviousMouseMode = CurrentMouseMode;
    CurrentMouseMode = NewMode;
    ApplyMouseModeSettings(NewMode);

    FString ModeName = MouseModeToString(CurrentMouseMode);
    FString ExtraInfo;

    if (CurrentMouseMode == EMAMouseMode::Modify)
    {
        ExtraInfo = TEXT(" [Click to select Actor, Shift+Click for multi-select]");
    }

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
        FString::Printf(TEXT("Mode: %s%s (, to toggle Modify)"), *ModeName, *ExtraInfo));

    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Mouse mode: %s"), *ModeName);
}

void AMAPlayerController::ApplyMouseModeSettings(EMAMouseMode Mode)
{
    // 所有模式都显示鼠标，视角旋转由右键控制
    bShowMouseCursor = true;
    SetIgnoreLookInput(true);  // 禁用默认视角控制，由右键拖动处理
    
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);

    // 处理 Modify 模式的进入/退出
    if (Mode == EMAMouseMode::Modify && PreviousMouseMode != EMAMouseMode::Modify)
    {
        // 进入 Modify 模式
        EnterModifyMode();
    }
    else if (Mode != EMAMouseMode::Modify && PreviousMouseMode == EMAMouseMode::Modify)
    {
        // 退出 Modify 模式
        ExitModifyMode();
    }

    // 处理 Edit 模式的进入/退出
    if (Mode == EMAMouseMode::Edit && PreviousMouseMode != EMAMouseMode::Edit)
    {
        // 进入 Edit 模式
        EnterEditMode();
    }
    else if (Mode != EMAMouseMode::Edit && PreviousMouseMode == EMAMouseMode::Edit)
    {
        // 退出 Edit 模式
        ExitEditMode();
    }
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
    if (Count <= 0) return;
    
    // 查找是否已有该类型
    for (FMAPendingDeployment& D : DeploymentQueue)
    {
        if (D.AgentType == AgentType)
        {
            D.Count += Count;
            OnDeploymentQueueChanged.Broadcast();
            UE_LOG(LogTemp, Log, TEXT("[PlayerController] Added %d x %s to queue (total: %d)"), 
                Count, *AgentType, D.Count);
            return;
        }
    }
    
    // 新类型
    DeploymentQueue.Add(FMAPendingDeployment(AgentType, Count));
    OnDeploymentQueueChanged.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Added new type %d x %s to queue"), Count, *AgentType);
}

void AMAPlayerController::RemoveFromDeploymentQueue(const FString& AgentType, int32 Count)
{
    if (Count <= 0) return;
    
    for (int32 i = DeploymentQueue.Num() - 1; i >= 0; --i)
    {
        if (DeploymentQueue[i].AgentType == AgentType)
        {
            DeploymentQueue[i].Count -= Count;
            if (DeploymentQueue[i].Count <= 0)
            {
                DeploymentQueue.RemoveAt(i);
            }
            OnDeploymentQueueChanged.Broadcast();
            return;
        }
    }
}

void AMAPlayerController::ClearDeploymentQueue()
{
    DeploymentQueue.Empty();
    CurrentDeploymentIndex = 0;
    OnDeploymentQueueChanged.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Deployment queue cleared"));
}

int32 AMAPlayerController::GetDeploymentQueueCount() const
{
    int32 Total = 0;
    for (const FMAPendingDeployment& D : DeploymentQueue)
    {
        Total += D.Count;
    }
    return Total;
}

FString AMAPlayerController::GetCurrentDeployingType() const
{
    if (CurrentDeploymentIndex < DeploymentQueue.Num())
    {
        return DeploymentQueue[CurrentDeploymentIndex].AgentType;
    }
    return TEXT("");
}

int32 AMAPlayerController::GetCurrentDeployingCount() const
{
    if (CurrentDeploymentIndex < DeploymentQueue.Num())
    {
        return DeploymentQueue[CurrentDeploymentIndex].Count;
    }
    return 0;
}

// ========== 部署模式 ==========

void AMAPlayerController::EnterDeploymentMode()
{
    if (!HasPendingDeployments())
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] EnterDeploymentMode: No units in queue"));
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("No units to deploy!"));
        return;
    }
    
    PreviousMouseMode = CurrentMouseMode;
    CurrentMouseMode = EMAMouseMode::Deployment;
    CurrentDeploymentIndex = 0;
    DeployedCount = 0;
    
    ApplyMouseModeSettings(EMAMouseMode::Deployment);
    
    int32 TotalCount = GetDeploymentQueueCount();
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Entered Deployment Mode: %d types, %d total agents"), 
        DeploymentQueue.Num(), TotalCount);
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
        FString::Printf(TEXT("Deployment Mode: Drag to place %d agents"), TotalCount));
}

void AMAPlayerController::EnterDeploymentModeWithUnits(const TArray<FMAPendingDeployment>& Deployments)
{
    // 添加到背包
    for (const FMAPendingDeployment& D : Deployments)
    {
        AddToDeploymentQueue(D.AgentType, D.Count);
    }
    
    // 进入部署模式
    EnterDeploymentMode();
}

void AMAPlayerController::ExitDeploymentMode()
{
    // 取消正在进行的框选
    if (SelectionManager && SelectionManager->IsBoxSelecting())
    {
        SelectionManager->CancelBoxSelect();
    }
    
    // 恢复之前的模式
    CurrentMouseMode = PreviousMouseMode;
    ApplyMouseModeSettings(CurrentMouseMode);
    
    int32 RemainingCount = GetDeploymentQueueCount();
    
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Exited Deployment Mode, deployed: %d, remaining: %d"), 
        DeployedCount, RemainingCount);
    
    if (RemainingCount > 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, 
            FString::Printf(TEXT("Deployment paused. %d units remaining in queue."), RemainingCount));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("All units deployed!"));
        // 广播完成事件
        OnDeploymentCompleted.Broadcast();
    }
    
    CurrentDeploymentIndex = 0;
    DeployedCount = 0;
}

void AMAPlayerController::OnDeploymentLeftClick()
{
    // 开始框选（复用 SelectionManager 的框选逻辑）
    if (SelectionManager)
    {
        float MouseX, MouseY;
        if (GetMousePosition(MouseX, MouseY))
        {
            SelectionManager->BeginBoxSelect(FVector2D(MouseX, MouseY));
        }
    }
}

void AMAPlayerController::OnDeploymentLeftClickReleased()
{
    if (!SelectionManager || !SelectionManager->IsBoxSelecting()) return;
    if (!AgentManager) return;
    
    // 更新终点
    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionManager->UpdateBoxSelect(FVector2D(MouseX, MouseY));
    }
    
    FVector2D Start = SelectionManager->GetBoxSelectStart();
    FVector2D End = SelectionManager->GetBoxSelectEnd();
    
    // 取消框选状态
    SelectionManager->CancelBoxSelect();
    
    // 计算框选大小
    float BoxWidth = FMath::Abs(End.X - Start.X);
    float BoxHeight = FMath::Abs(End.Y - Start.Y);
    
    // 框太小，忽略
    if (BoxWidth < 20.f && BoxHeight < 20.f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Drag a larger area to deploy"));
        return;
    }
    
    // 获取当前要部署的类型
    if (CurrentDeploymentIndex >= DeploymentQueue.Num())
    {
        ExitDeploymentMode();
        return;
    }
    
    FMAPendingDeployment& CurrentDeployment = DeploymentQueue[CurrentDeploymentIndex];
    int32 CountToSpawn = CurrentDeployment.Count;
    
    // 计算生成点
    TArray<FVector> SpawnPoints = ProjectSelectionBoxToWorld(Start, End, CountToSpawn);
    
    // 生成 Agent
    int32 SpawnedThisBatch = 0;
    for (const FVector& Point : SpawnPoints)
    {
        if (AgentManager->SpawnAgentByType(CurrentDeployment.AgentType, Point, FRotator::ZeroRotator, false))
        {
            SpawnedThisBatch++;
            DeployedCount++;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Deployed %d x %s"), SpawnedThisBatch, *CurrentDeployment.AgentType);
    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
        FString::Printf(TEXT("Deployed %d x %s"), SpawnedThisBatch, *CurrentDeployment.AgentType));
    
    // 从背包中移除已部署的
    DeploymentQueue.RemoveAt(CurrentDeploymentIndex);
    OnDeploymentQueueChanged.Broadcast();
    
    // 检查是否还有待部署的（不需要增加索引，因为已经移除了当前项）
    if (DeploymentQueue.Num() == 0)
    {
        ExitDeploymentMode();
    }
    else
    {
        // 确保索引有效
        if (CurrentDeploymentIndex >= DeploymentQueue.Num())
        {
            CurrentDeploymentIndex = 0;
        }
        
        FMAPendingDeployment& NextDeployment = DeploymentQueue[CurrentDeploymentIndex];
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
            FString::Printf(TEXT("Next: Drag to place %d x %s"), NextDeployment.Count, *NextDeployment.AgentType));
    }
}

TArray<FVector> AMAPlayerController::ProjectSelectionBoxToWorld(FVector2D Start, FVector2D End, int32 Count)
{
    TArray<FVector> Points;
    if (Count <= 0) return Points;
    
    // 计算屏幕框的边界
    float MinX = FMath::Min(Start.X, End.X);
    float MaxX = FMath::Max(Start.X, End.X);
    float MinY = FMath::Min(Start.Y, End.Y);
    float MaxY = FMath::Max(Start.Y, End.Y);
    
    float BoxWidth = MaxX - MinX;
    float BoxHeight = MaxY - MinY;
    
    UE_LOG(LogTemp, Warning, TEXT("[Deployment] Screen Box: (%.0f, %.0f) to (%.0f, %.0f), Size: %.0f x %.0f"),
        MinX, MinY, MaxX, MaxY, BoxWidth, BoxHeight);
    
    // 在屏幕空间网格分布，然后每个点单独投影到世界
    int32 Cols = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt((float)Count)));
    int32 Rows = FMath::Max(1, FMath::CeilToInt((float)Count / Cols));
    
    int32 Spawned = 0;
    for (int32 Row = 0; Row < Rows && Spawned < Count; ++Row)
    {
        for (int32 Col = 0; Col < Cols && Spawned < Count; ++Col)
        {
            // 计算屏幕空间的 UV
            float U = (Cols > 1) ? ((float)Col / (Cols - 1)) : 0.5f;
            float V = (Rows > 1) ? ((float)Row / (Rows - 1)) : 0.5f;
            
            // 添加一点随机偏移
            U = FMath::Clamp(U + FMath::RandRange(-0.05f, 0.05f), 0.f, 1.f);
            V = FMath::Clamp(V + FMath::RandRange(-0.05f, 0.05f), 0.f, 1.f);
            
            // 计算屏幕坐标
            float ScreenX = FMath::Lerp(MinX, MaxX, U);
            float ScreenY = FMath::Lerp(MinY, MaxY, V);
            
            // 从屏幕坐标射线检测地面
            FVector WorldPos, WorldDir;
            DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldPos, WorldDir);
            
            // 射线检测地面
            FHitResult HitResult;
            FVector TraceStart = WorldPos;
            FVector TraceEnd = WorldPos + WorldDir * 50000.f;  // 沿视线方向延伸
            
            FVector SpawnPoint;
            if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
            {
                SpawnPoint = HitResult.Location;
            }
            else
            {
                // 没有命中，使用 ProjectToGround 作为备选
                SpawnPoint = ProjectToGround(WorldPos + WorldDir * 5000.f);
            }
            
            UE_LOG(LogTemp, Warning, TEXT("[Deployment] SpawnPoint[%d]: Screen=(%.0f, %.0f) -> World=(%.0f, %.0f, %.0f)"),
                Spawned, ScreenX, ScreenY, SpawnPoint.X, SpawnPoint.Y, SpawnPoint.Z);
            
            Points.Add(SpawnPoint);
            Spawned++;
        }
    }

    return Points;
}

FVector AMAPlayerController::ProjectToGround(FVector WorldLocation)
{
    UWorld* World = GetWorld();
    if (!World) return WorldLocation;

    FHitResult HitResult;
    // 从固定高空位置开始检测，确保能检测到任何高度的地面
    // 使用 XY 坐标，但 Z 从 10000 开始向下检测到 -20000
    FVector TraceStart = FVector(WorldLocation.X, WorldLocation.Y, 10000.f);
    FVector TraceEnd = FVector(WorldLocation.X, WorldLocation.Y, -20000.f);

    if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
    {
        return HitResult.Location;
    }

    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] ProjectToGround: No ground at (%.0f, %.0f)"), 
        WorldLocation.X, WorldLocation.Y);
    return WorldLocation;
}

// ========== UI 切换 (Z 键) ==========

void AMAPlayerController::OnToggleMainUI(const FInputActionValue& Value)
{
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->ToggleMainUI();
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] ToggleMainUI called"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] MAHUD not found!"));
    }
}

void AMAPlayerController::OnToggleSkillAllocationViewer(const FInputActionValue& Value)
{
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->ToggleSkillAllocationViewer();
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] ToggleSkillAllocationViewer called"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] MAHUD not found!"));
    }
}

// ========== 突发事件系统 ==========

void AMAPlayerController::OnTriggerEmergency(const FInputActionValue& Value)
{
    if (!EmergencyManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] EmergencyManager not found!"));
        return;
    }
    
    // 切换事件状态（触发或结束）
    EmergencyManager->ToggleEvent();
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnTriggerEmergency called, event active: %s"), 
        EmergencyManager->IsEventActive() ? TEXT("true") : TEXT("false"));
}

void AMAPlayerController::OnToggleEmergencyUI(const FInputActionValue& Value)
{
    if (!EmergencyManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] EmergencyManager not found!"));
        return;
    }
    
    // 只有在事件激活时才能切换详情界面
    if (!EmergencyManager->IsEventActive())
    {
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnToggleEmergencyUI: No active emergency event"));
        return;
    }

    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->ToggleEmergencyWidget();
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnToggleEmergencyUI called"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] MAHUD not found!"));
    }
}

// ========== HUD 状态管理输入 (UI Visual Redesign) ==========
// Requirements: 10.4

void AMAPlayerController::OnCheckTask(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnCheckTask triggered (Z key pressed)"));
    
    if (!HUDStateManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnCheckTask: HUDStateManager is NULL! Attempting to re-acquire..."));
        
        // 尝试重新获取 HUDStateManager
        if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
        {
            if (UMAUIManager* UIManager = HUD->GetUIManager())
            {
                HUDStateManager = UIManager->GetHUDStateManager();
                if (HUDStateManager)
                {
                    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnCheckTask: HUDStateManager re-acquired successfully"));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[PlayerController] OnCheckTask: HUDStateManager still NULL after re-acquire attempt"));
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[PlayerController] OnCheckTask: UIManager is NULL"));
                return;
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[PlayerController] OnCheckTask: MAHUD is NULL"));
            return;
        }
    }

    HUDStateManager->HandleCheckTaskInput();
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnCheckTask -> HUDStateManager::HandleCheckTaskInput completed"));
}

void AMAPlayerController::OnCheckSkill(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnCheckSkill triggered (N key pressed)"));
    
    if (!HUDStateManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] OnCheckSkill: HUDStateManager is NULL! Attempting to re-acquire..."));
        
        // 尝试重新获取 HUDStateManager
        if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
        {
            if (UMAUIManager* UIManager = HUD->GetUIManager())
            {
                HUDStateManager = UIManager->GetHUDStateManager();
                if (HUDStateManager)
                {
                    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnCheckSkill: HUDStateManager re-acquired successfully"));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[PlayerController] OnCheckSkill: HUDStateManager still NULL after re-acquire attempt"));
                    return;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[PlayerController] OnCheckSkill: UIManager is NULL"));
                return;
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[PlayerController] OnCheckSkill: MAHUD is NULL"));
            return;
        }
    }

    HUDStateManager->HandleCheckSkillInput();
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnCheckSkill -> HUDStateManager::HandleCheckSkillInput completed"));
}

void AMAPlayerController::OnCheckUnexpected(const FInputActionValue& Value)
{
    if (!HUDStateManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] HUDStateManager not found!"));
        return;
    }

    HUDStateManager->HandleCheckEmergencyInput();
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnCheckUnexpected -> HUDStateManager::HandleCheckEmergencyInput"));
}

void AMAPlayerController::OnJumpPressed(const FInputActionValue& Value)
{
    if (!SelectionManager)
    {
        return;
    }

    TArray<AMACharacter*> SelectedAgents = SelectionManager->GetSelectedAgents();
    if (SelectedAgents.Num() == 0)
    {
        return;
    }

    int32 JumpCount = 0;
    for (AMACharacter* Agent : SelectedAgents)
    {
        if (Agent && Agent->CanJump())
        {
            Agent->Jump();
            JumpCount++;
        }
    }

    if (JumpCount > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] Jump: %d agents jumped"), JumpCount);
    }
}

// ========== Modify 模式 ==========

// 查找根 Actor（向上遍历 Attach 父级直到找到最顶层）
static AActor* FindRootActor(AActor* Actor)
{
    if (!Actor) return nullptr;

    AActor* Current = Actor;
    AActor* Parent = Current->GetAttachParentActor();

    // 向上遍历直到没有父 Actor
    while (Parent)
    {
        Current = Parent;
        Parent = Current->GetAttachParentActor();
    }

    return Current;
}

// 递归收集 Actor 及其所有子 Actor
static void CollectActorAndChildren(AActor* Actor, TArray<AActor*>& OutActors)
{
    if (!Actor) return;

    OutActors.Add(Actor);

    // 获取所有附加的子 Actor
    TArray<AActor*> AttachedActors;
    Actor->GetAttachedActors(AttachedActors);

    for (AActor* Child : AttachedActors)
    {
        CollectActorAndChildren(Child, OutActors);
    }
}

// 对单个 Actor 设置高亮（不递归）
void AMAPlayerController::SetSingleActorHighlight(AActor* Actor, bool bHighlight)
{
    if (!Actor) return;

    // 获取所有 PrimitiveComponent
    TArray<UPrimitiveComponent*> Components;
    Actor->GetComponents<UPrimitiveComponent>(Components);

    // 创建高亮材质的静态函数
    auto CreateHighlightMaterial = [](UObject* Outer, FLinearColor Color) -> UMaterialInstanceDynamic*
    {
        // 使用引擎的 Unlit 材质作为基础（纯色，不受光照影响）
        static UMaterial* UnlitMaterial = nullptr;
        if (!UnlitMaterial)
        {
            // 尝试多个可能的材质路径
            UnlitMaterial = LoadObject<UMaterial>(nullptr,
                TEXT("/Engine/EngineMaterials/DefaultDeferredDecalMaterial.DefaultDeferredDecalMaterial"));

            if (!UnlitMaterial)
            {
                UnlitMaterial = LoadObject<UMaterial>(nullptr,
                    TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
            }
        }

        if (UnlitMaterial)
        {
            UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(UnlitMaterial, Outer);
            if (DynMat)
            {
                // 尝试设置各种参数
                DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
                DynMat->SetVectorParameterValue(TEXT("Color"), Color);
                return DynMat;
            }
        }
        return nullptr;
    };

    // ========== 高亮颜色配置 ==========
    // 可以修改这里的颜色来尝试不同效果：
    // 鲜艳橙色: FLinearColor(1.0f, 0.3f, 0.0f)
    // 亮青色:   FLinearColor(0.0f, 1.0f, 1.0f)
    // 亮绿色:   FLinearColor(0.0f, 1.0f, 0.0f)
    // 亮粉色:   FLinearColor(1.0f, 0.0f, 0.5f)
    // 亮黄色:   FLinearColor(1.0f, 1.0f, 0.0f)
    // 亮蓝色:   FLinearColor(0.0f, 0.5f, 1.0f)
    // 纯白色:   FLinearColor(1.0f, 1.0f, 1.0f)
    FLinearColor HighlightColor(0.0f, 1.0f, 0.5f);  // 当前：亮青绿色
    // ====================================

    for (UPrimitiveComponent* Comp : Components)
    {
        if (Comp)
        {
            // 方式 1: Custom Depth (需要 Post Process 配合)
            Comp->SetRenderCustomDepth(bHighlight);
            Comp->SetCustomDepthStencilValue(bHighlight ? 1 : 0);

            // 方式 2: 使用 Overlay Material 实现高亮
            UMeshComponent* MeshComp = Cast<UMeshComponent>(Comp);
            if (MeshComp)
            {
                if (bHighlight)
                {
                    UMaterialInstanceDynamic* DynMat = CreateHighlightMaterial(MeshComp, HighlightColor);
                    if (DynMat)
                    {
                        MeshComp->SetOverlayMaterial(DynMat);
                    }
                }
                else
                {
                    // 清除 Overlay Material
                    MeshComp->SetOverlayMaterial(nullptr);
                }
            }
        }
    }
}

void AMAPlayerController::SetActorHighlight(AActor* Actor, bool bHighlight)
{
    if (!Actor) return;

    // 查找根 Actor
    AActor* RootActor = FindRootActor(Actor);

    // 收集根 Actor 及其所有子 Actor
    TArray<AActor*> AllActors;
    CollectActorAndChildren(RootActor, AllActors);

    // 对所有 Actor 设置高亮
    for (AActor* ActorToHighlight : AllActors)
    {
        SetSingleActorHighlight(ActorToHighlight, bHighlight);
    }

    UE_LOG(LogTemp, Log, TEXT("[PlayerController] SetActorHighlight: %s (Root: %s, Total: %d actors) -> %s"),
        *Actor->GetName(),
        *RootActor->GetName(),
        AllActors.Num(),
        bHighlight ? TEXT("ON") : TEXT("OFF"));
}

void AMAPlayerController::ClearAllHighlights()
{
    // 遍历所有高亮的 Actor 并清除高亮
    for (AActor* Actor : HighlightedActors)
    {
        if (Actor)
        {
            SetActorHighlight(Actor, false);
        }
    }

    // 清空数组
    HighlightedActors.Empty();

    UE_LOG(LogTemp, Log, TEXT("[PlayerController] ClearAllHighlights: Cleared all selections"));
}

void AMAPlayerController::AddToSelection(AActor* Actor)
{
    if (!Actor) return;

    // 查找根 Actor
    AActor* RootActor = FindRootActor(Actor);

    // 检查是否已在选择集中 - toggle 行为
    if (HighlightedActors.Contains(RootActor))
    {
        // 已存在则移除 (toggle)
        RemoveFromSelection(RootActor);
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] AddToSelection: Toggled off %s"), *RootActor->GetName());
    }
    else
    {
        // 不存在则添加
        HighlightedActors.Add(RootActor);
        SetActorHighlight(RootActor, true);
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] AddToSelection: Added %s (total: %d)"),
            *RootActor->GetName(), HighlightedActors.Num());
    }

    // 广播多选委托
    OnModifyActorsSelected.Broadcast(HighlightedActors);

    // 向后兼容：如果只有一个选中，也广播单选委托
    if (HighlightedActors.Num() == 1)
    {
        OnModifyActorSelected.Broadcast(HighlightedActors[0]);
    }
    else if (HighlightedActors.Num() == 0)
    {
        OnModifyActorSelected.Broadcast(nullptr);
    }
}

void AMAPlayerController::RemoveFromSelection(AActor* Actor)
{
    if (!Actor) return;

    // 查找根 Actor
    AActor* RootActor = FindRootActor(Actor);

    // 从选择集移除
    if (HighlightedActors.Remove(RootActor) > 0)
    {
        // 移除该 Actor 的高亮
        SetActorHighlight(RootActor, false);
        UE_LOG(LogTemp, Log, TEXT("[PlayerController] RemoveFromSelection: Removed %s (remaining: %d)"),
            *RootActor->GetName(), HighlightedActors.Num());

        // 广播多选委托
        OnModifyActorsSelected.Broadcast(HighlightedActors);

        // 向后兼容
        if (HighlightedActors.Num() == 1)
        {
            OnModifyActorSelected.Broadcast(HighlightedActors[0]);
        }
        else if (HighlightedActors.Num() == 0)
        {
            OnModifyActorSelected.Broadcast(nullptr);
        }
    }
}

void AMAPlayerController::ClearAndSelect(AActor* Actor)
{
    // 清除所有现有选择和高亮
    ClearAllHighlights();

    if (!Actor)
    {
        // 广播空选中事件
        OnModifyActorSelected.Broadcast(nullptr);
        OnModifyActorsSelected.Broadcast(HighlightedActors);
        return;
    }

    // 查找根 Actor
    AActor* RootActor = FindRootActor(Actor);

    // 选中并高亮单个 Actor
    HighlightedActors.Add(RootActor);
    SetActorHighlight(RootActor, true);

    UE_LOG(LogTemp, Log, TEXT("[PlayerController] ClearAndSelect: Selected %s"), *RootActor->GetName());

    // 广播选中事件
    OnModifyActorSelected.Broadcast(RootActor);
    OnModifyActorsSelected.Broadcast(HighlightedActors);
}

void AMAPlayerController::OnModifyLeftClick()
{
    // 检测 Shift 键状态
    bool bShiftPressed = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

    // 执行射线检测
    FHitResult HitResult;
    if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
    {
        AActor* HitActor = HitResult.GetActor();

        if (HitActor)
        {
            if (bShiftPressed)
            {
                // Shift+Click: 添加到选择集 (toggle 行为)
                AddToSelection(HitActor);
                UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnModifyLeftClick: Shift+Click on %s"),
                    *HitActor->GetName());
            }
            else
            {
                // 普通 Click: 清除所有选择并选中单个 Actor
                ClearAndSelect(HitActor);
                UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnModifyLeftClick: Click on %s"),
                    *HitActor->GetName());
            }
            return;
        }
    }

    // 点击空白区域，清除所有选择（无论是否按 Shift）
    ClearAllHighlights();

    // 广播空选中事件
    OnModifyActorSelected.Broadcast(nullptr);
    OnModifyActorsSelected.Broadcast(HighlightedActors);

    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnModifyLeftClick: Clicked empty area, cleared all selections"));
}

void AMAPlayerController::EnterModifyMode()
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] EnterModifyMode"));

    // 通知 HUD 显示 ModifyWidget
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->ShowModifyWidget();
    }
}

void AMAPlayerController::ExitModifyMode()
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] ExitModifyMode"));

    // 清除高亮
    ClearAllHighlights();

    // 通知 HUD 隐藏 ModifyWidget
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->HideModifyWidget();
    }
}

void AMAPlayerController::ClearModifyHighlight()
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] ClearModifyHighlight"));

    // 清除所有高亮
    ClearAllHighlights();

    // 广播空选中事件
    OnModifyActorSelected.Broadcast(nullptr);
    OnModifyActorsSelected.Broadcast(HighlightedActors);
}


// ========== Edit 模式 ==========

void AMAPlayerController::OnEditLeftClick()
{

    // 检查鼠标是否在 UI 上，如果是则不处理场景交互
    // 这样 UMG 按钮可以正常接收点击，同时避免在 UI 区域创建 POI
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        if (HUD->IsMouseOverEditWidget())
        {
            UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnEditLeftClick: Mouse over UI, skipping scene interaction"));
            return;
        }
    }

    // 检测 Shift 键状态
    bool bShiftPressed = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

    if (bShiftPressed)
    {
        // Shift+Click: 选择 Actor 或 POI
        FHitResult HitResult;
        if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
        {
            AActor* HitActor = HitResult.GetActor();

            if (HitActor)
            {
                // 检查是否点击了 POI
                AMAPointOfInterest* POI = Cast<AMAPointOfInterest>(HitActor);
                if (POI)
                {
                    // 点击 POI: 切换选择状态
                    if (EditModeManager)
                    {
                        if (EditModeManager->GetSelectedPOIs().Contains(POI))
                        {
                            // 已选中，取消选择
                            EditModeManager->DeselectObject(POI);
                            UE_LOG(LogTemp, Log, TEXT("[PlayerController] Edit Mode: Deselected POI at %s"),
                                *POI->GetActorLocation().ToString());
                        }
                        else
                        {
                            // 未选中，添加到选择
                            EditModeManager->SelectPOI(POI);
                            UE_LOG(LogTemp, Log, TEXT("[PlayerController] Edit Mode: Selected POI at %s"),
                                *POI->GetActorLocation().ToString());
                        }
                    }
                    return;
                }

                // 点击普通 Actor: 选择 Actor (单选)
                if (EditModeManager)
                {
                    if (EditModeManager->GetSelectedActor() == HitActor)
                    {
                        // 已选中，取消选择
                        EditModeManager->DeselectObject(HitActor);
                        UE_LOG(LogTemp, Log, TEXT("[PlayerController] Edit Mode: Deselected Actor %s"),
                            *HitActor->GetName());
                    }
                    else
                    {
                        // 选择新 Actor
                        EditModeManager->SelectActor(HitActor);
                        UE_LOG(LogTemp, Log, TEXT("[PlayerController] Edit Mode: Selected Actor %s"),
                            *HitActor->GetName());
                    }
                }
                return;
            }
        }

        // 点击空白区域，清除选择
        if (EditModeManager)
        {
            EditModeManager->ClearSelection();
            UE_LOG(LogTemp, Log, TEXT("[PlayerController] Edit Mode: Cleared selection"));
        }
    }
    else
    {
        // 普通 Click: 创建 POI
        FVector HitLocation;
        if (GetMouseHitLocation(HitLocation))
        {
            if (EditModeManager)
            {
                AMAPointOfInterest* NewPOI = EditModeManager->CreatePOI(HitLocation);
                if (NewPOI)
                {
                    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Edit Mode: Created POI at %s"),
                        *HitLocation.ToString());

                    // 在屏幕上显示坐标
                    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                        FString::Printf(TEXT("POI: (%.0f, %.0f, %.0f)"),
                            HitLocation.X, HitLocation.Y, HitLocation.Z));
                }
            }
        }
    }
}

void AMAPlayerController::EnterEditMode()
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] EnterEditMode"));

    // 检查 Edit Mode 是否可用
    if (EditModeManager && !EditModeManager->IsEditModeAvailable())
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Edit Mode is not available (source scene graph not found)"));
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
            TEXT("Edit Mode unavailable: source scene graph file not found"));
        return;
    }

    // 通知 HUD 显示 EditWidget
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->ShowEditWidget();
    }
}

void AMAPlayerController::ExitEditMode()
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] ExitEditMode"));

    // 清除所有 POI 和选择
    if (EditModeManager)
    {
        EditModeManager->ClearSelection();
        EditModeManager->DestroyAllPOIs();
    }

    // 通知 HUD 隐藏 EditWidget
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->HideEditWidget();
    }
}

void AMAPlayerController::OnToggleViewportRecording(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] OnToggleViewportRecording"));

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UMAViewportRecorder* ViewportRecorder = World->GetSubsystem<UMAViewportRecorder>();
    if (ViewportRecorder)
    {
        ViewportRecorder->ToggleRecording();
        
        if (ViewportRecorder->IsRecording())
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Viewport Recording Started"));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, 
                FString::Printf(TEXT("Viewport Recording Stopped (%d frames)"), ViewportRecorder->GetRecordedFrameCount()));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] ViewportRecorder subsystem not found"));
    }
}
