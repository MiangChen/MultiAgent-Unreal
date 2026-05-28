// MAUAVCharacter.cpp
// 多旋翼无人机实现
// 飞行控制由 MANavigationService 中的 FlightController 统一管理

#include "MAUAVCharacter.h"
#include "Agent/CharacterRuntime/Bootstrap/MACharacterRuntimeBootstrap.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"
#include "../../StateTree/Runtime/MAStateTreeComponent.h"
#include "Agent/Navigation/Runtime/MANavigationService.h"
#include "StateTree.h"

AMAUAVCharacter::AMAUAVCharacter()
{
    AgentType = EMAAgentType::UAV;
    
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(false);
    
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->SetMovementMode(MOVE_Flying);
    MovementComp->DefaultLandMovementMode = MOVE_Flying;
    MovementComp->MaxFlySpeed = 800.f;
    MovementComp->GravityScale = 0.f;
    
    // 加载 Inspire 2 模型
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
        TEXT("/Game/Robot/dji_inspire2/dji_inspire2.dji_inspire2"));
    if (MeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(MeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -75.f));
        GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    }

    // 加载螺旋桨动画
    static ConstructorHelpers::FObjectFinder<UAnimSequence> PropellerAnimAsset(
        TEXT("/Game/Robot/dji_inspire2/dji_inspire2_2Hovering.dji_inspire2_2Hovering"));
    if (PropellerAnimAsset.Succeeded())
    {
        PropellerAnim = PropellerAnimAsset.Object;
    }
}

void AMAUAVCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    FMACharacterRuntimeBootstrap::ApplyUAVFlightConfig(*this);
    
    if (SkillComponent)
    {
        SkillComponent->OnEnergyDepleted.AddDynamic(this, &AMAUAVCharacter::OnEnergyDepleted);
        SkillComponent->EnergyDrainRate = 0.5f;
    }
    
    // 尝试加载 StateTree 资产（仅当 StateTree 启用时）
    if (StateTreeComponent && StateTreeComponent->IsStateTreeEnabled())
    {
        if (!StateTreeComponent->HasStateTree())
        {
            UStateTree* ST = LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_UAV.ST_UAV"));
            if (ST)
            {
                StateTreeComponent->SetStateTreeAsset(ST);
                UE_LOG(LogTemp, Log, TEXT("[UAV %s] StateTree loaded: ST_UAV"), *AgentID);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[UAV %s] StateTree asset not found: /Game/StateTree/ST_UAV.ST_UAV"), *AgentID);
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("[UAV %s] StateTree already set"), *AgentID);
        }
    }
    else if (StateTreeComponent && !StateTreeComponent->IsStateTreeEnabled())
    {
        UE_LOG(LogTemp, Log, TEXT("[UAV %s] StateTree disabled, using direct skill activation"), *AgentID);
    }
    
    // 初始时螺旋桨不转，等待起飞
    if (PropellerAnim)
    {
        GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        GetMesh()->SetAnimation(PropellerAnim);
        GetMesh()->Stop();
    }
}

void AMAUAVCharacter::InitializeSkillSet()
{
    AvailableSkills.Add(EMASkillType::Navigate);
    AvailableSkills.Add(EMASkillType::Search);
    AvailableSkills.Add(EMASkillType::Follow);
    AvailableSkills.Add(EMASkillType::Charge);
}

void AMAUAVCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 更新螺旋桨动画（根据导航状态）
    UpdatePropellerAnimation();
    
    // 能量消耗（在空中时）
    if (IsInAir() && ShouldDrainEnergy() && SkillComponent && SkillComponent->HasEnergy())
    {
        SkillComponent->DrainEnergy(DeltaTime);
    }
}

void AMAUAVCharacter::UpdatePropellerAnimation()
{
    if (!PropellerAnim) return;
    
    // 根据导航服务状态控制螺旋桨
    bool bShouldSpin = IsInAir();
    
    USkeletalMeshComponent* Mesh = GetMesh();
    if (!Mesh) return;
    
    if (bShouldSpin && !Mesh->IsPlaying())
    {
        Mesh->Play(true);
    }
    else if (!bShouldSpin && Mesh->IsPlaying())
    {
        Mesh->Stop();
    }
}

bool AMAUAVCharacter::IsInAir() const
{
    // 检查当前高度是否高于地面一定距离
    FVector CurrentLocation = GetActorLocation();
    FVector Start = CurrentLocation;
    FVector End = Start - FVector(0.f, 0.f, 200.f);
    
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    
    bool bGroundNearby = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);
    
    // 如果200cm内没有地面，认为在空中
    return !bGroundNearby;
}

void AMAUAVCharacter::OnEnergyDepleted()
{
    // 能量耗尽时，取消当前导航
    if (NavigationService)
    {
        NavigationService->CancelNavigation();
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[UAV %s] Energy depleted!"), *AgentLabel);
}
