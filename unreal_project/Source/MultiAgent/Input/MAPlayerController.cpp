// MAPlayerController.cpp (重构版)
// 使用 Enhanced Input System + MACommandManager
// 支持星际争霸风格的框选和编组
// 支持 Modify 模式的场景标注

#include "MAPlayerController.h"
#include "MAInputActions.h"
#include "../Core/Manager/MAAgentManager.h"
#include "../UI/MAHUD.h"
#include "../Core/Manager/MACommandManager.h"
#include "../Core/Manager/MASquadManager.h"
#include "../Core/MASquad.h"
#include "../Core/Manager/MASelectionManager.h"
#include "../Core/Manager/MAViewportManager.h"
#include "../Core/Manager/MAEmergencyManager.h"
#include "../Core/Manager/MAEditModeManager.h"
#include "../Environment/MAPointOfInterest.h"
#include "../UI/MASelectionHUD.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Character/MAQuadrupedCharacter.h"
#include "../Agent/Character/MAUAVCharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Canvas.h"
#include "Components/PrimitiveComponent.h"

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
    EmergencyManager = World->GetSubsystem<UMAEmergencyManager>();
    EditModeManager = World->GetSubsystem<UMAEditModeManager>();

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

        // UI 切换 (Z 键)
        EIC->BindAction(InputActions->IA_ToggleMainUI, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleMainUI);

        // 突发事件系统
        EIC->BindAction(InputActions->IA_TriggerEmergency, ETriggerEvent::Started, this, &AMAPlayerController::OnTriggerEmergency);
        EIC->BindAction(InputActions->IA_ToggleEmergencyUI, ETriggerEvent::Started, this, &AMAPlayerController::OnToggleEmergencyUI);

        // 跳跃 (空格键)
        EIC->BindAction(InputActions->IA_Jump, ETriggerEvent::Started, this, &AMAPlayerController::OnJumpPressed);
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
    // 根据当前模式分发处理
    switch (CurrentMouseMode)
    {
    case EMAMouseMode::Deployment:
        OnDeploymentLeftClick();
        return;
        
    case EMAMouseMode::Modify:
        OnModifyLeftClick();
        return;
        
    case EMAMouseMode::Edit:
        OnEditLeftClick();
        return;
        
    case EMAMouseMode::Select:
    default:
        // Select 模式：开始框选
        if (SelectionManager)
        {
            float MouseX, MouseY;
            if (GetMousePosition(MouseX, MouseY))
            {
                SelectionManager->BeginBoxSelect(FVector2D(MouseX, MouseY));
            }
        }
        break;
    }
}

void AMAPlayerController::OnLeftClickReleased(const FInputActionValue& Value)
{
    if (CurrentMouseMode == EMAMouseMode::Deployment)
    {
        OnDeploymentLeftClickReleased();
        return;
    }
    
    // Modify 模式不需要处理释放事件
    if (CurrentMouseMode == EMAMouseMode::Modify)
    {
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
        // 点击空地：不清除选择
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

    // 更新 HUD 的框选状态
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

            if (Delta.Size() > 2.f)
            {
                if (APawn* ControlledPawn = GetPawn())
                {
                    FRotator NewRotation = ControlledPawn->GetActorRotation();
                    NewRotation.Yaw += Delta.X * CameraRotationSensitivity;
                    NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch - Delta.Y * CameraRotationSensitivity, -89.f, 89.f);
                    ControlledPawn->SetActorRotation(NewRotation);
                    SetControlRotation(NewRotation);
                }
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

    bIsRightMouseRotating = true;
    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY))
    {
        RightMouseStartPosition = FVector2D(MouseX, MouseY);
    }
}

void AMAPlayerController::OnRightClickReleased(const FInputActionValue& Value)
{
    bIsRightMouseRotating = false;
}

void AMAPlayerController::OnMiddleClick(const FInputActionValue& Value)
{
    if (!SelectionManager) return;

    TArray<AMACharacter*> SelectedAgents = SelectionManager->GetSelectedAgents();
    if (SelectedAgents.Num() == 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("No agents selected"));
        return;
    }

    FVector HitLocation;
    if (!GetMouseHitLocation(HitLocation)) return;

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
        SelectionManager->SaveToControlGroup(GroupIndex);
    }
    else
    {
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
    if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift)) return;

    if (!SelectionManager || !SquadManager) return;

    TArray<AMACharacter*> Selected = SelectionManager->GetSelectedAgents();
    if (Selected.Num() < 2)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, 
            TEXT("Select at least 2 agents to create squad (Q)"));
        return;
    }

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
    if (!IsInputKeyDown(EKeys::LeftShift) && !IsInputKeyDown(EKeys::RightShift)) return;

    if (!SelectionManager || !SquadManager) return;

    TArray<AMACharacter*> Selected = SelectionManager->GetSelectedAgents();

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
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("No squad to disband (Shift+Q)"));
        return;
    }

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
    // 切换: Select -> Deployment (如果有待部署单位) -> Modify -> Edit -> Select
    EMAMouseMode NewMode;
    
    switch (CurrentMouseMode)
    {
    case EMAMouseMode::Select:
        // 如果背包有待部署单位，切换到部署模式，否则跳过到 Modify
        if (HasPendingDeployments())
        {
            NewMode = EMAMouseMode::Deployment;
        }
        else
        {
            NewMode = EMAMouseMode::Modify;
        }
        break;
        
    case EMAMouseMode::Deployment:
        NewMode = EMAMouseMode::Modify;
        break;
        
    case EMAMouseMode::Modify:
        NewMode = EMAMouseMode::Edit;
        break;
        
    case EMAMouseMode::Edit:
        NewMode = EMAMouseMode::Select;
        break;
        
    default:
        NewMode = EMAMouseMode::Select;
        break;
    }

    // 退出当前模式
    if (CurrentMouseMode == EMAMouseMode::Deployment)
    {
        if (SelectionManager && SelectionManager->IsBoxSelecting())
        {
            SelectionManager->CancelBoxSelect();
        }
        CurrentDeploymentIndex = 0;
    }
    else if (CurrentMouseMode == EMAMouseMode::Modify)
    {
        ExitModifyMode();
    }
    else if (CurrentMouseMode == EMAMouseMode::Edit)
    {
        ExitEditMode();
    }

    // 进入新模式
    if (NewMode == EMAMouseMode::Deployment)
    {
        CurrentDeploymentIndex = 0;
        DeployedCount = 0;
    }
    else if (NewMode == EMAMouseMode::Modify)
    {
        EnterModifyMode();
    }
    else if (NewMode == EMAMouseMode::Edit)
    {
        EnterEditMode();
    }

    PreviousMouseMode = CurrentMouseMode;
    CurrentMouseMode = NewMode;
    ApplyMouseModeSettings(NewMode);

    FString ModeName = MouseModeToString(CurrentMouseMode);
    FString ExtraInfo;
    
    if (CurrentMouseMode == EMAMouseMode::Deployment)
    {
        ExtraInfo = FString::Printf(TEXT(" [%d pending]"), GetDeploymentQueueCount());
    }
    else if (CurrentMouseMode == EMAMouseMode::Modify)
    {
        ExtraInfo = TEXT(" [Click to select Actor, Shift+Click for multi-select]");
    }
    else if (CurrentMouseMode == EMAMouseMode::Edit)
    {
        ExtraInfo = TEXT(" [Click to create POI, Shift+Click to select]");
    }

    GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
        FString::Printf(TEXT("Mode: %s%s (M to switch)"), *ModeName, *ExtraInfo));
    
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Mouse mode: %s"), *ModeName);
}

void AMAPlayerController::ApplyMouseModeSettings(EMAMouseMode Mode)
{
    bShowMouseCursor = true;
    SetIgnoreLookInput(true);

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
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
    for (const FMAPendingDeployment& D : Deployments)
    {
        AddToDeploymentQueue(D.AgentType, D.Count);
    }
    EnterDeploymentMode();
}

void AMAPlayerController::ExitDeploymentMode()
{
    if (SelectionManager && SelectionManager->IsBoxSelecting())
    {
        SelectionManager->CancelBoxSelect();
    }

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
        OnDeploymentCompleted.Broadcast();
    }

    CurrentDeploymentIndex = 0;
    DeployedCount = 0;
}

void AMAPlayerController::OnDeploymentLeftClick()
{
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

    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionManager->UpdateBoxSelect(FVector2D(MouseX, MouseY));
    }

    FVector2D Start = SelectionManager->GetBoxSelectStart();
    FVector2D End = SelectionManager->GetBoxSelectEnd();

    SelectionManager->CancelBoxSelect();

    float BoxWidth = FMath::Abs(End.X - Start.X);
    float BoxHeight = FMath::Abs(End.Y - Start.Y);

    if (BoxWidth < 20.f && BoxHeight < 20.f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Drag a larger area to deploy"));
        return;
    }

    if (CurrentDeploymentIndex >= DeploymentQueue.Num())
    {
        ExitDeploymentMode();
        return;
    }

    FMAPendingDeployment& CurrentDeployment = DeploymentQueue[CurrentDeploymentIndex];
    int32 CountToSpawn = CurrentDeployment.Count;

    TArray<FVector> SpawnPoints = ProjectSelectionBoxToWorld(Start, End, CountToSpawn);

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

    DeploymentQueue.RemoveAt(CurrentDeploymentIndex);
    OnDeploymentQueueChanged.Broadcast();

    if (DeploymentQueue.Num() == 0)
    {
        ExitDeploymentMode();
    }
    else
    {
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

    float MinX = FMath::Min(Start.X, End.X);
    float MaxX = FMath::Max(Start.X, End.X);
    float MinY = FMath::Min(Start.Y, End.Y);
    float MaxY = FMath::Max(Start.Y, End.Y);
    float BoxWidth = MaxX - MinX;
    float BoxHeight = MaxY - MinY;

    int32 Cols = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt((float)Count)));
    int32 Rows = FMath::Max(1, FMath::CeilToInt((float)Count / Cols));

    int32 Spawned = 0;
    for (int32 Row = 0; Row < Rows && Spawned < Count; ++Row)
    {
        for (int32 Col = 0; Col < Cols && Spawned < Count; ++Col)
        {
            float U = (Cols > 1) ? ((float)Col / (Cols - 1)) : 0.5f;
            float V = (Rows > 1) ? ((float)Row / (Rows - 1)) : 0.5f;

            U = FMath::Clamp(U + FMath::RandRange(-0.05f, 0.05f), 0.f, 1.f);
            V = FMath::Clamp(V + FMath::RandRange(-0.05f, 0.05f), 0.f, 1.f);

            float ScreenX = FMath::Lerp(MinX, MaxX, U);
            float ScreenY = FMath::Lerp(MinY, MaxY, V);

            FVector WorldPos, WorldDir;
            DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldPos, WorldDir);

            FHitResult HitResult;
            FVector TraceStart = WorldPos;
            FVector TraceEnd = WorldPos + WorldDir * 50000.f;

            FVector SpawnPoint;
            if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
            {
                SpawnPoint = HitResult.Location;
            }
            else
            {
                SpawnPoint = ProjectToGround(WorldPos + WorldDir * 5000.f);
            }

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


// ========== Modify 模式 ==========

void AMAPlayerController::EnterModifyMode()
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Entering Modify mode"));
    
    // 显示 ModifyWidget
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->ShowModifyWidget();
    }
}

void AMAPlayerController::ExitModifyMode()
{
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Exiting Modify mode"));
    
    // 清除所有高亮
    ClearAllHighlights();
    
    // 隐藏 ModifyWidget
    if (AMAHUD* HUD = Cast<AMAHUD>(GetHUD()))
    {
        HUD->HideModifyWidget();
    }
}

void AMAPlayerController::OnModifyLeftClick()
{
    // 射线检测点击的 Actor
    FHitResult HitResult;
    if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
    {
        // 点击空地：如果没有按 Shift，清除选择
        bool bShiftPressed = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
        if (!bShiftPressed)
        {
            ClearAllHighlights();
            
            // 广播空选择
            OnModifyActorSelected.Broadcast(nullptr);
            OnModifyActorsSelected.Broadcast(HighlightedActors);
        }
        return;
    }
    
    AActor* HitActor = HitResult.GetActor();
    if (!HitActor)
    {
        return;
    }
    
    // 检查是否按住 Shift
    bool bShiftPressed = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    
    if (bShiftPressed)
    {
        // Shift+Click: 切换选择状态 (toggle)
        AddToSelection(HitActor);
    }
    else
    {
        // 普通点击: 清除之前的选择，选中新的
        ClearAndSelect(HitActor);
    }
    
    // 广播选择变化
    if (HighlightedActors.Num() == 1)
    {
        OnModifyActorSelected.Broadcast(HighlightedActors[0]);
    }
    else if (HighlightedActors.Num() > 1)
    {
        // 多选时也广播单选委托（使用第一个）
        OnModifyActorSelected.Broadcast(HighlightedActors[0]);
    }
    else
    {
        OnModifyActorSelected.Broadcast(nullptr);
    }
    
    // 始终广播多选委托
    OnModifyActorsSelected.Broadcast(HighlightedActors);
    
    UE_LOG(LogTemp, Log, TEXT("[PlayerController] Modify selection: %d actors"), HighlightedActors.Num());
}

void AMAPlayerController::AddToSelection(AActor* Actor)
{
    if (!Actor) return;
    
    // 检查是否已经在选择集合中
    if (HighlightedActors.Contains(Actor))
    {
        // 已选中，移除（toggle 行为）
        RemoveFromSelection(Actor);
    }
    else
    {
        // 未选中，添加
        HighlightedActors.Add(Actor);
        SetActorHighlight(Actor, true);
    }
}

void AMAPlayerController::RemoveFromSelection(AActor* Actor)
{
    if (!Actor) return;
    
    if (HighlightedActors.Remove(Actor) > 0)
    {
        SetActorHighlight(Actor, false);
    }
}

void AMAPlayerController::ClearAndSelect(AActor* Actor)
{
    // 清除所有现有高亮
    ClearAllHighlights();
    
    // 选中新的 Actor
    if (Actor)
    {
        HighlightedActors.Add(Actor);
        SetActorHighlight(Actor, true);
    }
}

void AMAPlayerController::ClearAllHighlights()
{
    for (AActor* Actor : HighlightedActors)
    {
        if (Actor)
        {
            SetActorHighlight(Actor, false);
        }
    }
    HighlightedActors.Empty();
}

void AMAPlayerController::ClearModifyHighlight()
{
    ClearAllHighlights();
    
    // 广播空选择
    OnModifyActorSelected.Broadcast(nullptr);
    OnModifyActorsSelected.Broadcast(HighlightedActors);
}

void AMAPlayerController::SetActorHighlight(AActor* Actor, bool bHighlight)
{
    if (!Actor) return;
    
    // 查找根 Actor（如果是子 Actor）
    AActor* RootActor = Actor->GetAttachParentActor();
    if (!RootActor)
    {
        RootActor = Actor;
    }
    
    // 高亮根 Actor 及其所有子 Actor
    SetSingleActorHighlight(RootActor, bHighlight);
    
    TArray<AActor*> ChildActors;
    RootActor->GetAttachedActors(ChildActors, true);  // 递归获取所有子 Actor
    
    for (AActor* Child : ChildActors)
    {
        SetSingleActorHighlight(Child, bHighlight);
    }
}

// 对单个 Actor 设置高亮（不递归）
void AMAPlayerController::SetSingleActorHighlight(AActor* Actor, bool bHighlight)
{
    if (!Actor) return;
    
    // 获取所有 PrimitiveComponent
    TArray<UPrimitiveComponent*> Components;
    Actor->GetComponents<UPrimitiveComponent>(Components);
    
    // 创建高亮材质的 lambda
    auto CreateHighlightMaterial = [](UObject* Outer, FLinearColor Color) -> UMaterialInstanceDynamic*
    {
        static UMaterial* UnlitMaterial = nullptr;
        if (!UnlitMaterial)
        {
            // 优先使用 DeferredDecal 材质（效果更明显）
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
                // 同时设置两个参数，确保兼容性
                DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
                DynMat->SetVectorParameterValue(TEXT("Color"), Color);
                return DynMat;
            }
        }
        return nullptr;
    };
    
    // 高亮颜色 - 不带 alpha，完全不透明
    FLinearColor HighlightColor(0.0f, 1.0f, 0.5f);  // 亮青绿色
    
    for (UPrimitiveComponent* Comp : Components)
    {
        if (Comp)
        {
            // Custom Depth
            Comp->SetRenderCustomDepth(bHighlight);
            Comp->SetCustomDepthStencilValue(bHighlight ? 1 : 0);
            
            // Overlay Material
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
                    MeshComp->SetOverlayMaterial(nullptr);
                }
            }
        }
    }
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

// ========== 突发事件系统 ==========

void AMAPlayerController::OnTriggerEmergency(const FInputActionValue& Value)
{
    if (!EmergencyManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerController] EmergencyManager not found!"));
        return;
    }

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


// ========== Edit 模式 ==========

void AMAPlayerController::OnEditLeftClick()
{
    // 检查鼠标是否在 UI 上，如果是则不处理场景交互
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
                            EditModeManager->DeselectObject(POI);
                            UE_LOG(LogTemp, Log, TEXT("[PlayerController] Edit Mode: Deselected POI at %s"), 
                                *POI->GetActorLocation().ToString());
                        }
                        else
                        {
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
                        EditModeManager->DeselectObject(HitActor);
                        UE_LOG(LogTemp, Log, TEXT("[PlayerController] Edit Mode: Deselected Actor %s"), 
                            *HitActor->GetName());
                    }
                    else
                    {
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
            TEXT("Edit Mode 不可用: 源场景图文件未找到"));
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
