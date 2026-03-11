// MACharacter.cpp
// 机器人角色基类实现

#include "MACharacter.h"
#include "Agent/CharacterRuntime/Application/MACharacterRuntimeUseCases.h"
#include "Agent/CharacterRuntime/Infrastructure/MACharacterRuntimeBridge.h"
#include "Agent/Skill/Application/MASkillActivationUseCases.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "Agent/Skill/Domain/MASkillTags.h"
#include "Agent/Sensing/Runtime/MACameraSensorComponent.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "Kismet/GameplayStatics.h"
#include "Components/WidgetComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "UI/Components/Presentation/MASpeechBubbleWidget.h"

AMACharacter::AMACharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    
    AgentID = TEXT("");
    AgentLabel = TEXT("Agent");
    AgentType = EMAAgentType::Humanoid;
    bIsMoving = false;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);  // 允许射线检测（Modify 模式点击选中）

    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);  // 更快的转向速度
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    GetCharacterMovement()->bUseRVOAvoidance = true;
    GetCharacterMovement()->MaxStepHeight = 100.f;

    // 创建技能组件
    SkillComponent = CreateDefaultSubobject<UMASkillComponent>(TEXT("SkillComponent"));
    
    // 创建导航服务组件
    NavigationService = CreateDefaultSubobject<UMANavigationService>(TEXT("NavigationService"));
    
    // 创建气泡文本框组件（World 空间，跟随场景缩放）
    SpeechBubbleComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SpeechBubbleComponent"));
    SpeechBubbleComponent->SetupAttachment(GetRootComponent());
    SpeechBubbleComponent->SetWidgetSpace(EWidgetSpace::World);
    SpeechBubbleComponent->SetDrawSize(UMASpeechBubbleWidget::GetRecommendedDrawSize());
    SpeechBubbleComponent->SetPivot(UMASpeechBubbleWidget::GetRecommendedPivot(UMASpeechBubbleWidget::GetRecommendedDrawSize().X));  // 引脚尖端自动对齐 robot 头顶
    SpeechBubbleComponent->SetVisibility(false);
    SpeechBubbleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpeechBubbleComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    SpeechBubbleComponent->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));  // 放缩到合适的世界尺寸
    SpeechBubbleComponent->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));  // 透明背景，让圆角和阴影正确渲染
    SpeechBubbleComponent->SetBlendMode(EWidgetBlendMode::Transparent);  // 透明混合模式
    SpeechBubbleComponent->SetTintColorAndOpacity(FLinearColor(0.01f, 0.01f, 0.01f, 1.0f));  // 降低整体亮度，避免场景光照导致过曝
    
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

UAbilitySystemComponent* AMACharacter::GetAbilitySystemComponent() const
{
    return SkillComponent;
}

void AMACharacter::BeginPlay()
{
    Super::BeginPlay();
    FMASkillTags::InitializeNativeTags();
    AutoFitCapsuleToMesh();
    InitializeSkillSet();
    InitializeSpeechBubble();
    
    // 记录初始位置（用于返航）
    InitialLocation = GetActorLocation();
    
    // 绑定低电量自动返航
    if (SkillComponent)
    {
        SkillComponent->OnLowEnergy.AddDynamic(this, &AMACharacter::OnLowEnergyReturn);
    }
}

void AMACharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (SkillComponent)
    {
        SkillComponent->InitAbilityActorInfo(this, this);
        SkillComponent->InitializeSkills(this);
    }
}

void AMACharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateStatusText();
    UpdateSpeechBubbleFacing();
}

// ========== 技能接口 ==========

bool AMACharacter::TryNavigateTo(FVector Destination)
{
    if (SkillComponent)
    {
        return FMASkillActivationUseCases::PrepareAndActivateNavigate(*SkillComponent, Destination);
    }
    return false;
}

void AMACharacter::CancelNavigation()
{
    if (SkillComponent)
    {
        FMASkillActivationUseCases::CancelCommand(*SkillComponent, EMACommand::Navigate);
    }
}

bool AMACharacter::TryFollowActor(AActor* TargetActor, float FollowDistance)
{
    if (SkillComponent)
    {
        return FMASkillActivationUseCases::PrepareAndActivateFollow(*SkillComponent, TargetActor, FollowDistance);
    }
    return false;
}

void AMACharacter::StopFollowing()
{
    if (SkillComponent)
    {
        FMASkillActivationUseCases::CancelCommand(*SkillComponent, EMACommand::Follow);
    }
}

// ========== 直接控制 ==========

void AMACharacter::SetDirectControl(bool bEnabled)
{
    if (bIsUnderDirectControl == bEnabled) return;
    
    bIsUnderDirectControl = bEnabled;
    
    if (bEnabled)
    {
        CancelAIMovement();
        GetCharacterMovement()->bOrientRotationToMovement = false;
    }
    else
    {
        GetCharacterMovement()->bOrientRotationToMovement = true;
    }
}

void AMACharacter::CancelAIMovement()
{
    AAIController* AIController = Cast<AAIController>(GetController());
    if (AIController)
    {
        AIController->StopMovement();
        if (UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent())
        {
            PathFollowing->AbortMove(*this, FPathFollowingResultFlags::UserAbort);
        }
    }
    
    if (SkillComponent)
    {
        FMASkillActivationUseCases::CancelCommand(*SkillComponent, EMACommand::Navigate);
        FMASkillActivationUseCases::CancelCommand(*SkillComponent, EMACommand::Follow);
    }
    
    bIsMoving = false;
}

void AMACharacter::ApplyDirectMovement(FVector WorldDirection)
{
    if (!bIsUnderDirectControl) return;
    
    if (!WorldDirection.IsNearlyZero())
    {
        AAIController* AIController = Cast<AAIController>(GetController());
        if (AIController && AIController->GetMoveStatus() != EPathFollowingStatus::Idle)
        {
            AIController->StopMovement();
        }
    }
    
    AddMovementInput(WorldDirection, 1.0f, false);
}

// ========== 头顶状态显示 ==========

void AMACharacter::ShowStatus(const FString& Text, float Duration)
{
    CurrentStatusText = Text;
    StatusDisplayEndTime = GetWorld()->GetTimeSeconds() + Duration;
}

void AMACharacter::ShowAbilityStatus(const FString& SkillName, const FString& Params)
{
    FString DisplayText = Params.IsEmpty() 
        ? FString::Printf(TEXT("[%s]"), *SkillName)
        : FString::Printf(TEXT("[%s] %s"), *SkillName, *Params);
    ShowStatus(DisplayText, 3.0f);
}

FString AMACharacter::GetCurrentStatusText() const
{
    return CurrentStatusText;
}

void AMACharacter::UpdateStatusText()
{
    if (CurrentStatusText.IsEmpty()) return;
    if (GetWorld()->GetTimeSeconds() > StatusDisplayEndTime)
    {
        CurrentStatusText = TEXT("");
    }
    // 状态文字由 HUD 绘制，这里只负责更新过期状态
}

// ========== Sensor 管理 ==========

UMACameraSensorComponent* AMACharacter::AddCameraSensor(FVector RelativeLocation, FRotator RelativeRotation)
{
    static int32 SensorCounter = 0;
    FName ComponentName = *FString::Printf(TEXT("CameraSensor_%d"), SensorCounter++);
    UMACameraSensorComponent* CameraSensor = NewObject<UMACameraSensorComponent>(this, ComponentName);
    
    if (CameraSensor)
    {
        CameraSensor->SetupAttachment(GetRootComponent());
        CameraSensor->SetRelativeLocation(RelativeLocation);
        CameraSensor->SetRelativeRotation(RelativeRotation);
        CameraSensor->RegisterComponent();
        CameraSensor->SensorName = FString::Printf(TEXT("%s_Camera"), *AgentLabel);
    }
    
    return CameraSensor;
}

UMACameraSensorComponent* AMACharacter::GetCameraSensor() const
{
    return FindComponentByClass<UMACameraSensorComponent>();
}

int32 AMACharacter::GetSensorCount() const
{
    TArray<UMACameraSensorComponent*> Sensors;
    GetComponents<UMACameraSensorComponent>(Sensors);
    return Sensors.Num();
}

// ========== 辅助方法 ==========

void AMACharacter::AutoFitCapsuleToMesh()
{
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp || !MeshComp->GetSkeletalMeshAsset()) return;
    
    FBoxSphereBounds LocalBounds = MeshComp->GetSkeletalMeshAsset()->GetBounds();
    float ExtentX = LocalBounds.BoxExtent.X;
    float ExtentY = LocalBounds.BoxExtent.Y;
    float ExtentZ = LocalBounds.BoxExtent.Z;
    
    float Radius = (ExtentX + ExtentY) * 0.5f * 0.8f;
    float HalfHeight = ExtentZ * 0.85f;
    
    Radius = FMath::Max(Radius, 10.f);
    HalfHeight = FMath::Max(HalfHeight, Radius);
    
    GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
    
    float MeshLocalBottomZ = LocalBounds.Origin.Z - ExtentZ;
    float MeshOffsetZ = -HalfHeight - MeshLocalBottomZ;
    
    FVector CurrentOffset = MeshComp->GetRelativeLocation();
    MeshComp->SetRelativeLocation(FVector(CurrentOffset.X, CurrentOffset.Y, MeshOffsetZ));
}

bool AMACharacter::ShouldDrainEnergy() const
{
    return FMACharacterRuntimeBridge::ShouldDrainEnergy(this);
}

void AMACharacter::OnLowEnergyReturn()
{
    if (!SkillComponent) return;
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] Low energy (%.0f%%), initiating return to base"),
        *AgentLabel, SkillComponent->GetEnergyPercent());
    
    ShowStatus(TEXT("[Low Battery] Returning to base..."), 5.f);
    
    // 检查是否处于暂停状态
    if (FMACharacterRuntimeBridge::IsExecutionPaused(this))
    {
        // 暂停中：先取消当前技能，标记待返航，等恢复后执行
        SkillComponent->CancelAllSkills();
        bPendingLowEnergyReturn = true;
        FMACharacterRuntimeBridge::BindPauseStateChanged(*this);
        UE_LOG(LogTemp, Log, TEXT("[%s] Paused, deferring low-energy return until resume"), *AgentLabel);
        return;
    }
    
    ExecuteLowEnergyReturn();
}

void AMACharacter::OnPauseStateChanged(bool bPaused)
{
    if (!bPaused && bPendingLowEnergyReturn)
    {
        bPendingLowEnergyReturn = false;
        
        // 解绑，只需要触发一次
        FMACharacterRuntimeBridge::UnbindPauseStateChanged(*this);
        
        ExecuteLowEnergyReturn();
    }
}

void AMACharacter::ExecuteLowEnergyReturn()
{
    if (!SkillComponent) return;
    
    // 取消当前所有技能（不算成功完成）
    SkillComponent->CancelAllSkills();
    
    // 标记正在低电量返航（条件检查系统会豁免 BatteryLow 检查）
    bIsLowEnergyReturning = true;
    
    // 监听返航技能完成，清除标记
    SkillComponent->OnSkillCompleted.AddDynamic(this, &AMACharacter::OnReturnSkillCompleted);

    const FMACharacterRuntimeFeedback ReturnPlan =
        FMACharacterRuntimeUseCases::BuildLowEnergyReturnPlan(
            NavigationService && NavigationService->bIsFlying);

    if (ReturnPlan.ReturnMode == EMACharacterReturnMode::ReturnHome)
    {
        SkillComponent->GetSkillParamsMutable().HomeLocation = InitialLocation;
        FMASkillActivationUseCases::ActivatePreparedCommand(*SkillComponent, EMACommand::ReturnHome);
    }
    else
    {
        FMASkillActivationUseCases::PrepareAndActivateNavigate(*SkillComponent, InitialLocation);
    }
    
    UE_LOG(LogTemp, Log, TEXT("[%s] %s ([%.0f, %.0f, %.0f])"),
        *AgentLabel, *ReturnPlan.Message,
        InitialLocation.X, InitialLocation.Y, InitialLocation.Z);
}

void AMACharacter::OnReturnSkillCompleted(AMACharacter* Agent, bool bSuccess, const FString& Message)
{
    if (Agent != this) return;
    
    bIsLowEnergyReturning = false;
    
    if (SkillComponent)
    {
        SkillComponent->OnSkillCompleted.RemoveDynamic(this, &AMACharacter::OnReturnSkillCompleted);
    }
    
    UE_LOG(LogTemp, Log, TEXT("[%s] Low-energy return %s: %s"),
        *AgentLabel, bSuccess ? TEXT("completed") : TEXT("failed"), *Message);
}

// ========== 气泡文本框 ==========

void AMACharacter::UpdateSpeechBubbleFacing()
{
    if (!SpeechBubbleComponent || !SpeechBubbleComponent->IsVisible()) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->PlayerCameraManager) return;

    FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
    FVector BubbleLoc = SpeechBubbleComponent->GetComponentLocation();
    FVector Dir = CameraLoc - BubbleLoc;
    Dir.Z = 0.0f;

    if (!Dir.IsNearlyZero())
    {
        FRotator FaceRot = Dir.Rotation();
        SpeechBubbleComponent->SetWorldRotation(FaceRot);
    }
}

void AMACharacter::InitializeSpeechBubble()
{
    if (!SpeechBubbleComponent) return;

    SpeechBubbleComponent->SetWidgetClass(UMASpeechBubbleWidget::StaticClass());
    SpeechBubbleComponent->InitWidget();

    // 应用主题
    UMASpeechBubbleWidget* BubbleWidget = Cast<UMASpeechBubbleWidget>(SpeechBubbleComponent->GetWidget());
    if (BubbleWidget)
    {
        FMACharacterRuntimeBridge::ApplySpeechBubbleTheme(this, *BubbleWidget);
    }
}

void AMACharacter::ShowSpeechBubble(const FString& Message, float Duration)
{
    if (!SpeechBubbleComponent) return;

    UMASpeechBubbleWidget* BubbleWidget = Cast<UMASpeechBubbleWidget>(SpeechBubbleComponent->GetWidget());
    if (!BubbleWidget) return;

    SpeechBubbleComponent->SetVisibility(true);
    BubbleWidget->ShowMessage(Message, Duration);
}

void AMACharacter::HideSpeechBubble()
{
    if (!SpeechBubbleComponent) return;

    UMASpeechBubbleWidget* BubbleWidget = Cast<UMASpeechBubbleWidget>(SpeechBubbleComponent->GetWidget());
    if (BubbleWidget)
    {
        BubbleWidget->HideMessage();
    }
}

UMASpeechBubbleWidget* AMACharacter::GetSpeechBubbleWidget() const
{
    if (!SpeechBubbleComponent) return nullptr;
    return Cast<UMASpeechBubbleWidget>(SpeechBubbleComponent->GetWidget());
}
