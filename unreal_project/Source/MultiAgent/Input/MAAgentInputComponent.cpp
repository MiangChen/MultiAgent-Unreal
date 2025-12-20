// MAAgentInputComponent.cpp

#include "MAAgentInputComponent.h"
#include "MAInputActions.h"
#include "../Agent/Character/MACharacter.h"
#include "../Agent/Character/MADroneCharacter.h"
#include "../Agent/Component/Sensor/MACameraSensorComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"

UMAAgentInputComponent::UMAAgentInputComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMAAgentInputComponent::Initialize(APlayerController* PC, AMACharacter* Agent)
{
    if (!PC || !Agent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AgentInputComponent] Initialize failed: PC or Agent is null"));
        return;
    }
    
    OwningPC = PC;
    ControlledAgent = Agent;
    
    // 重置相机俯仰角
    CurrentCameraPitch = -15.f;
    
    // 添加 IMC_AgentControl (优先级 1，高于 IMC_RTS)
    UMAInputActions* InputActions = UMAInputActions::Get();
    if (!InputActions || !InputActions->IMC_AgentControl)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AgentInputComponent] IMC_AgentControl not found"));
        return;
    }
    
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
    {
        Subsystem->AddMappingContext(InputActions->IMC_AgentControl, 1);
        UE_LOG(LogTemp, Log, TEXT("[AgentInputComponent] Added IMC_AgentControl (priority 1)"));
    }
    
    // 绑定输入
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent))
    {
        EIC->BindAction(InputActions->IA_Move, ETriggerEvent::Triggered, this, &UMAAgentInputComponent::OnMoveInput);
        EIC->BindAction(InputActions->IA_Look, ETriggerEvent::Triggered, this, &UMAAgentInputComponent::OnLookInput);
        EIC->BindAction(InputActions->IA_MoveUp, ETriggerEvent::Triggered, this, &UMAAgentInputComponent::OnMoveUp);
        EIC->BindAction(InputActions->IA_MoveDown, ETriggerEvent::Triggered, this, &UMAAgentInputComponent::OnMoveDown);
        EIC->BindAction(InputActions->IA_LookArrow, ETriggerEvent::Triggered, this, &UMAAgentInputComponent::OnLookArrowInput);
        EIC->BindAction(InputActions->IA_Jump, ETriggerEvent::Triggered, this, &UMAAgentInputComponent::OnJump);
        
        UE_LOG(LogTemp, Log, TEXT("[AgentInputComponent] Input bindings created for %s"), *Agent->AgentName);
    }
}

void UMAAgentInputComponent::Cleanup()
{
    // 移除 IMC_AgentControl
    UMAInputActions* InputActions = UMAInputActions::Get();
    if (InputActions && InputActions->IMC_AgentControl && OwningPC.IsValid())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwningPC->GetLocalPlayer()))
        {
            Subsystem->RemoveMappingContext(InputActions->IMC_AgentControl);
            UE_LOG(LogTemp, Log, TEXT("[AgentInputComponent] Removed IMC_AgentControl"));
        }
    }
    
    // 注意：输入绑定会在组件销毁时自动清理
    OwningPC.Reset();
    ControlledAgent.Reset();
}

void UMAAgentInputComponent::OnMoveInput(const FInputActionValue& Value)
{
    if (!ControlledAgent.IsValid()) return;
    
    AMACharacter* Agent = ControlledAgent.Get();
    FVector2D Input = Value.Get<FVector2D>();
    
    if (Input.IsNearlyZero()) return;
    
    // 获取 Agent 的朝向
    FRotator AgentRotation = Agent->GetActorRotation();
    
    // 计算基于 Agent 朝向的移动方向
    FVector ForwardDirection = FRotationMatrix(AgentRotation).GetUnitAxis(EAxis::X);
    FVector RightDirection = FRotationMatrix(AgentRotation).GetUnitAxis(EAxis::Y);
    
    // 组合移动方向 (Input.Y = 前后, Input.X = 左右)
    FVector WorldDirection = ForwardDirection * Input.Y + RightDirection * Input.X;
    WorldDirection.Z = 0.f;
    
    if (!WorldDirection.IsNearlyZero())
    {
        WorldDirection.Normalize();
    }
    
    Agent->ApplyDirectMovement(WorldDirection);
}

void UMAAgentInputComponent::OnLookInput(const FInputActionValue& Value)
{
    if (!ControlledAgent.IsValid()) return;
    
    AMACharacter* Agent = ControlledAgent.Get();
    FVector2D Input = Value.Get<FVector2D>();
    
    // 水平输入旋转 Agent Yaw
    if (!FMath::IsNearlyZero(Input.X))
    {
        FRotator CurrentRotation = Agent->GetActorRotation();
        CurrentRotation.Yaw += Input.X * LookSensitivityYaw;
        Agent->SetActorRotation(CurrentRotation);
    }
    
    // 垂直输入调整相机 Pitch
    if (!FMath::IsNearlyZero(Input.Y))
    {
        CurrentCameraPitch = FMath::Clamp(
            CurrentCameraPitch + Input.Y * LookSensitivityPitch,
            MinCameraPitch,
            MaxCameraPitch
        );
        
        UMACameraSensorComponent* CameraSensor = Agent->GetCameraSensor();
        if (CameraSensor)
        {
            UCameraComponent* CameraComp = CameraSensor->GetCameraComponent();
            if (CameraComp)
            {
                FRotator CameraRotation = CameraComp->GetRelativeRotation();
                CameraRotation.Pitch = CurrentCameraPitch;
                CameraComp->SetRelativeRotation(CameraRotation);
            }
        }
    }
}

void UMAAgentInputComponent::OnMoveUp(const FInputActionValue& Value)
{
    if (!ControlledAgent.IsValid()) return;
    
    // E 键用于 Drone 上升（需要在空中）
    if (AMADroneCharacter* Drone = Cast<AMADroneCharacter>(ControlledAgent.Get()))
    {
        if (Drone->IsLanded())
        {
            // 在地面时，E 键起飞
            Drone->TakeOff();
        }
        else
        {
            // 在空中时，E 键上升
            Drone->ApplyVerticalMovement(1.0f);
        }
    }
}

void UMAAgentInputComponent::OnMoveDown(const FInputActionValue& Value)
{
    if (!ControlledAgent.IsValid()) return;
    
    // Q 键用于 Drone 下降/降落
    if (AMADroneCharacter* Drone = Cast<AMADroneCharacter>(ControlledAgent.Get()))
    {
        if (Drone->IsInAir())
        {
            Drone->ApplyVerticalMovement(-1.0f);
        }
    }
}

void UMAAgentInputComponent::OnJump(const FInputActionValue& Value)
{
    if (!ControlledAgent.IsValid()) return;
    
    AMACharacter* Agent = ControlledAgent.Get();
    
    // Drone: Space 起飞或上升
    if (AMADroneCharacter* Drone = Cast<AMADroneCharacter>(Agent))
    {
        if (Drone->IsLanded())
        {
            // 在地面时，Space 起飞
            Drone->TakeOff();
        }
        else
        {
            // 在空中时，Space 上升
            Drone->ApplyVerticalMovement(1.0f);
        }
    }
    else
    {
        // 地面单位执行跳跃
        if (Agent->CanPerformJump())
        {
            Agent->PerformJump();
        }
    }
}

void UMAAgentInputComponent::OnLookArrowInput(const FInputActionValue& Value)
{
    if (!ControlledAgent.IsValid()) return;
    
    // 方向键视角，灵敏度降低
    FVector2D Input = Value.Get<FVector2D>();
    Input *= 0.125f;
    
    AMACharacter* Agent = ControlledAgent.Get();
    
    // 水平旋转
    if (!FMath::IsNearlyZero(Input.X))
    {
        FRotator CurrentRotation = Agent->GetActorRotation();
        CurrentRotation.Yaw += Input.X * LookSensitivityYaw;
        Agent->SetActorRotation(CurrentRotation);
    }
    
    // 垂直俯仰
    if (!FMath::IsNearlyZero(Input.Y))
    {
        CurrentCameraPitch = FMath::Clamp(
            CurrentCameraPitch + Input.Y * LookSensitivityPitch,
            MinCameraPitch,
            MaxCameraPitch
        );
        
        UMACameraSensorComponent* CameraSensor = Agent->GetCameraSensor();
        if (CameraSensor)
        {
            UCameraComponent* CameraComp = CameraSensor->GetCameraComponent();
            if (CameraComp)
            {
                FRotator CameraRotation = CameraComp->GetRelativeRotation();
                CameraRotation.Pitch = CurrentCameraPitch;
                CameraComp->SetRelativeRotation(CameraRotation);
            }
        }
    }
}
