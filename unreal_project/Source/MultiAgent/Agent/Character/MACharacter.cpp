// MACharacter.cpp
// 机器人角色基类实现

#include "MACharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "../Skill/MASkillComponent.h"
#include "../Skill/MASkillTags.h"
#include "../Component/Sensor/MACameraSensorComponent.h"
#include "../Component/MANavigationService.h"
#include "Kismet/GameplayStatics.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../../Core/Manager/MACommandManager.h"
#include "../../UI/Core/MAUIManager.h"
#include "../../UI/HUD/MAHUD.h"

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
}

// ========== 技能接口 ==========

bool AMACharacter::TryNavigateTo(FVector Destination)
{
    if (SkillComponent)
    {
        return SkillComponent->TryActivateNavigate(Destination);
    }
    return false;
}

void AMACharacter::CancelNavigation()
{
    if (SkillComponent)
    {
        SkillComponent->CancelNavigate();
    }
}

bool AMACharacter::TryFollowActor(AActor* TargetActor, float FollowDistance)
{
    if (SkillComponent)
    {
        return SkillComponent->TryActivateFollow(TargetActor, FollowDistance);
    }
    return false;
}

void AMACharacter::StopFollowing()
{
    if (SkillComponent)
    {
        SkillComponent->CancelFollow();
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
        SkillComponent->CancelNavigate();
        SkillComponent->CancelFollow();
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
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UMAConfigManager* ConfigMgr = GI->GetSubsystem<UMAConfigManager>())
        {
            return ConfigMgr->bEnableEnergyDrain;
        }
    }
    return false;
}

void AMACharacter::OnLowEnergyReturn()
{
    if (!SkillComponent) return;
    
    UE_LOG(LogTemp, Warning, TEXT("[%s] Low energy (%.0f%%), initiating return to base"),
        *AgentLabel, SkillComponent->GetEnergyPercent());
    
    ShowStatus(TEXT("[Low Battery] Returning to base..."), 5.f);
    
    // 检查是否处于暂停状态
    if (UMACommandManager* CmdMgr = GetWorld()->GetSubsystem<UMACommandManager>())
    {
        if (CmdMgr->IsPaused())
        {
            // 暂停中：先取消当前技能，标记待返航，等恢复后执行
            SkillComponent->CancelAllSkills();
            bPendingLowEnergyReturn = true;
            CmdMgr->OnExecutionPauseStateChanged.AddDynamic(this, &AMACharacter::OnPauseStateChanged);
            UE_LOG(LogTemp, Log, TEXT("[%s] Paused, deferring low-energy return until resume"), *AgentLabel);
            return;
        }
    }
    
    ExecuteLowEnergyReturn();
}

void AMACharacter::OnPauseStateChanged(bool bPaused)
{
    if (!bPaused && bPendingLowEnergyReturn)
    {
        bPendingLowEnergyReturn = false;
        
        // 解绑，只需要触发一次
        if (UMACommandManager* CmdMgr = GetWorld()->GetSubsystem<UMACommandManager>())
        {
            CmdMgr->OnExecutionPauseStateChanged.RemoveDynamic(this, &AMACharacter::OnPauseStateChanged);
        }
        
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
    
    // 飞行机器人：ReturnHome（含降落逻辑），地面机器人：Navigate 回初始位置
    bool bIsFlying = NavigationService && NavigationService->bIsFlying;
    
    if (bIsFlying)
    {
        SkillComponent->GetSkillParamsMutable().HomeLocation = InitialLocation;
        SkillComponent->TryActivateReturnHome();
    }
    else
    {
        SkillComponent->TryActivateNavigate(InitialLocation);
    }
    
    UE_LOG(LogTemp, Log, TEXT("[%s] Low-energy return started (%s → [%.0f, %.0f, %.0f])"),
        *AgentLabel, bIsFlying ? TEXT("ReturnHome") : TEXT("Navigate"),
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
