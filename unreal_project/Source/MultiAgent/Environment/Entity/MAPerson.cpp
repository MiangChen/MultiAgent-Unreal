// MAPerson.cpp
// 行人环境对象实现
// 使用 Population_System 资产包

#include "MAPerson.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "../../Agent/Component/MANavigationService.h"
#include "../../Core/Config/MAConfigManager.h"
#include "../Effect/MAFire.h"
#include "AIController.h"

AMAPerson::AMAPerson()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建导航服务组件
    NavigationService = CreateDefaultSubobject<UMANavigationService>(TEXT("NavigationService"));

    // 移动设置
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = 200.f;  // 行人速度较慢
    GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);

    // 启用碰撞阻挡
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // Mesh 设置
    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
    GetMesh()->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);

    // 设置 AI 控制
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    // 加载动画资源
    LoadAnimations();
}

void AMAPerson::BeginPlay()
{
    Super::BeginPlay();

    // 绑定导航完成回调
    if (NavigationService)
    {
        NavigationService->OnNavigationCompleted.AddDynamic(this, &AMAPerson::OnNavigationCompleted);
    }

    // 播放初始动画
    SetAnimation(CurrentAnimation);
}

void AMAPerson::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 根据移动速度自动切换动画（不依赖导航状态）
    float Speed = GetVelocity().Size();
    
    // 速度阈值
    constexpr float WalkThreshold = 10.f;
    constexpr float RunThreshold = 350.f;
    
    if (Speed > RunThreshold)
    {
        // 跑步
        if (CurrentAnimation != EMAPersonAnimation::Run)
        {
            SetAnimation(EMAPersonAnimation::Run);
        }
    }
    else if (Speed > WalkThreshold)
    {
        // 行走
        if (CurrentAnimation != EMAPersonAnimation::Walk)
        {
            SetAnimation(EMAPersonAnimation::Walk);
        }
    }
    else
    {
        // 静止 - 只有在之前是移动状态时才切换到 Idle
        if (bIsPlayingWalk)
        {
            SetAnimation(EMAPersonAnimation::Idle);
        }
    }
}

//=============================================================================
// IMAEnvironmentObject 接口实现
//=============================================================================

FString AMAPerson::GetObjectLabel() const { return ObjectLabel; }
FString AMAPerson::GetObjectType() const { return ObjectType; }
const TMap<FString, FString>& AMAPerson::GetObjectFeatures() const { return Features; }

//=============================================================================
// 配置方法
//=============================================================================

void AMAPerson::Configure(const FMAEnvironmentObjectConfig& Config)
{
    ObjectLabel = Config.Label;
    ObjectType = Config.Type;
    Features = Config.Features;
    SetActorRotation(Config.Rotation);

    // 根据 subtype, variant 和 gender 选择角色模型
    FString Subtype = Features.FindRef(TEXT("subtype"));
    FString Variant = Features.FindRef(TEXT("variant"));
    FString Gender = Features.FindRef(TEXT("gender"));
    if (Subtype.IsEmpty()) Subtype = TEXT("casual");
    if (Variant.IsEmpty()) Variant = TEXT("01");
    if (Gender.IsEmpty()) Gender = TEXT("male");
    SelectCharacterMesh(Subtype, Variant, Gender);

    // 设置初始动画
    FString AnimStr = Features.FindRef(TEXT("animation"));
    if (AnimStr.Equals(TEXT("walk"), ESearchCase::IgnoreCase))
    {
        SetAnimation(EMAPersonAnimation::Walk);
    }
    else if (AnimStr.Equals(TEXT("talk"), ESearchCase::IgnoreCase))
    {
        SetAnimation(EMAPersonAnimation::Talk);
    }
    else if (AnimStr.Equals(TEXT("run"), ESearchCase::IgnoreCase))
    {
        SetAnimation(EMAPersonAnimation::Run);
    }
    else
    {
        SetAnimation(EMAPersonAnimation::Idle);
    }

    // 配置巡逻
    if (Config.Patrol.bEnabled && Config.Patrol.Waypoints.Num() > 0)
    {
        PatrolWaypoints = Config.Patrol.Waypoints;
        bPatrolLoop = Config.Patrol.bLoop;
        PatrolWaitTime = Config.Patrol.WaitTime;
        
        // 延迟启动巡逻，等待 BeginPlay 完成
        if (UWorld* World = GetWorld())
        {
            FTimerHandle StartTimerHandle;
            World->GetTimerManager().SetTimer(StartTimerHandle, this, &AMAPerson::StartPatrol, 1.0f, false);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[MAPerson] Configured: %s (subtype=%s, variant=%s, gender=%s, patrol=%s)"), 
        *ObjectLabel, *Subtype, *Variant, *Gender, Config.Patrol.bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void AMAPerson::SetAnimation(EMAPersonAnimation Animation)
{
    CurrentAnimation = Animation;
    UAnimSequence* TargetAnim = nullptr;

    switch (Animation)
    {
    case EMAPersonAnimation::Idle:
        TargetAnim = IdleAnim;
        break;
    case EMAPersonAnimation::Walk:
        TargetAnim = WalkAnim;
        break;
    case EMAPersonAnimation::Talk:
        TargetAnim = TalkAnim ? TalkAnim : IdleAnim;
        break;
    case EMAPersonAnimation::Run:
        TargetAnim = RunAnim ? RunAnim : WalkAnim;
        break;
    }

    if (TargetAnim && GetMesh())
    {
        GetMesh()->SetAnimation(TargetAnim);
        GetMesh()->Play(true);
        bIsPlayingWalk = (Animation == EMAPersonAnimation::Walk || Animation == EMAPersonAnimation::Run);
    }
}

//=============================================================================
// 导航接口
//=============================================================================

bool AMAPerson::NavigateTo(FVector Destination, float AcceptanceRadius)
{
    if (!NavigationService)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAPerson] %s: NavigationService not available"), *ObjectLabel);
        return false;
    }

    bool bStarted = NavigationService->NavigateTo(Destination, AcceptanceRadius);
    if (bStarted)
    {
        SetAnimation(EMAPersonAnimation::Walk);
    }
    return bStarted;
}

void AMAPerson::CancelNavigation()
{
    if (NavigationService)
    {
        NavigationService->CancelNavigation();
        SetAnimation(EMAPersonAnimation::Idle);
    }
}

bool AMAPerson::IsNavigating() const
{
    return NavigationService && NavigationService->IsNavigating();
}

void AMAPerson::OnNavigationCompleted(bool bSuccess, const FString& Message)
{
    // 如果正在巡逻，处理巡逻逻辑
    if (bIsPatrolling)
    {
        if (bSuccess)
        {
            // 到达巡逻点，等待后前往下一个
            SetAnimation(EMAPersonAnimation::Idle);
            bIsPlayingWalk = false;
            
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(PatrolWaitTimerHandle, this, &AMAPerson::MoveToNextWaypoint, PatrolWaitTime, false);
            }
        }
        else
        {
            // 导航失败，尝试下一个点
            MoveToNextWaypoint();
        }
        return;
    }
    
    // 非巡逻模式的正常回调
    SetAnimation(EMAPersonAnimation::Idle);
    bIsPlayingWalk = false;
    
    UE_LOG(LogTemp, Log, TEXT("[MAPerson] %s: Navigation %s - %s"), 
        *ObjectLabel, bSuccess ? TEXT("succeeded") : TEXT("failed"), *Message);
}

//=============================================================================
// 私有方法
//=============================================================================

void AMAPerson::SelectCharacterMesh(const FString& Subtype, const FString& Variant, const FString& Gender)
{
    FString GenderFolder = Gender.Equals(TEXT("female"), ESearchCase::IgnoreCase) ? TEXT("Girls") : TEXT("Mans");
    FString GenderSuffix = Gender.Equals(TEXT("female"), ESearchCase::IgnoreCase) ? TEXT("f") : TEXT("m");
    
    FString SubtypeLower = Subtype.ToLower();
    FString MeshName = FString::Printf(TEXT("%s%s_%s"), *SubtypeLower, *Variant, *GenderSuffix);
    
    FString MeshPath = FString::Printf(
        TEXT("/Game/Population_System/Characters/%s/%s/%s.%s"),
        *GenderFolder, *MeshName, *MeshName, *MeshName);

    USkeletalMesh* LoadedMesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath);
    
    if (!LoadedMesh)
    {
        // 尝试备选路径 (casual01 作为默认)
        UE_LOG(LogTemp, Warning, TEXT("[MAPerson] Failed to load mesh: %s, falling back to casual01"), *MeshPath);
        MeshName = FString::Printf(TEXT("casual01_%s"), *GenderSuffix);
        MeshPath = FString::Printf(
            TEXT("/Game/Population_System/Characters/%s/%s/%s.%s"),
            *GenderFolder, *MeshName, *MeshName, *MeshName);
        LoadedMesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath);
        
        if (!LoadedMesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("[MAPerson] Failed to load fallback mesh: %s"), *MeshPath);
            return;
        }
    }

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetSkeletalMesh(LoadedMesh);
    }
}

void AMAPerson::LoadAnimations()
{
    // 加载动画资源
    static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAnimAsset(
        TEXT("/Game/Population_System/Animations/Mans/Man_01/man_idle1.man_idle1"));
    if (IdleAnimAsset.Succeeded())
    {
        IdleAnim = IdleAnimAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAnimAsset(
        TEXT("/Game/Population_System/Animations/Mans/Man_01/man_walk.man_walk"));
    if (WalkAnimAsset.Succeeded())
    {
        WalkAnim = WalkAnimAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequence> RunAnimAsset(
        TEXT("/Game/Population_System/Animations/Mans/Man_01/man_run.man_run"));
    if (RunAnimAsset.Succeeded())
    {
        RunAnim = RunAnimAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimSequence> TalkAnimAsset(
        TEXT("/Game/Population_System/Animations/Mans/Man_01/man_talk1.man_talk1"));
    if (TalkAnimAsset.Succeeded())
    {
        TalkAnim = TalkAnimAsset.Object;
    }
}

//=============================================================================
// 巡逻功能
//=============================================================================

void AMAPerson::StartPatrol()
{
    if (PatrolWaypoints.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MAPerson] %s: No patrol waypoints configured"), *ObjectLabel);
        return;
    }
    
    bIsPatrolling = true;
    CurrentWaypointIndex = 0;
    MoveToNextWaypoint();
    
    UE_LOG(LogTemp, Log, TEXT("[MAPerson] %s: Started patrol with %d waypoints"), *ObjectLabel, PatrolWaypoints.Num());
}

void AMAPerson::StopPatrol()
{
    bIsPatrolling = false;
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PatrolWaitTimerHandle);
    }
    
    CancelNavigation();
    UE_LOG(LogTemp, Log, TEXT("[MAPerson] %s: Stopped patrol"), *ObjectLabel);
}

void AMAPerson::MoveToNextWaypoint()
{
    if (!bIsPatrolling || PatrolWaypoints.Num() == 0) return;
    
    // 获取当前目标点
    FVector TargetLocation = PatrolWaypoints[CurrentWaypointIndex];
    
    // 更新索引到下一个点
    CurrentWaypointIndex++;
    if (CurrentWaypointIndex >= PatrolWaypoints.Num())
    {
        if (bPatrolLoop)
        {
            CurrentWaypointIndex = 0;  // 循环回第一个点
        }
        else
        {
            bIsPatrolling = false;
            UE_LOG(LogTemp, Log, TEXT("[MAPerson] %s: Patrol completed"), *ObjectLabel);
            return;
        }
    }
    
    // 导航到目标点
    NavigateTo(TargetLocation, 100.f);
}

//=============================================================================
// 附着火焰管理
//=============================================================================

void AMAPerson::SetAttachedFire(AMAFire* Fire)
{
    AttachedFire = Fire;
}

AMAFire* AMAPerson::GetAttachedFire() const
{
    return AttachedFire.Get();
}
