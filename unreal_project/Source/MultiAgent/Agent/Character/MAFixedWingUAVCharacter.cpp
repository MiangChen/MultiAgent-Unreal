// MAFixedWingUAVCharacter.cpp
// 固定翼无人机实现
// 飞行控制由 MANavigationService 中的 FlightController 统一管理

#include "MAFixedWingUAVCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "../Skill/MASkillComponent.h"
#include "../StateTree/MAStateTreeComponent.h"
#include "../Component/MANavigationService.h"
#include "StateTree.h"

AMAFixedWingUAVCharacter::AMAFixedWingUAVCharacter()
{
    AgentType = EMAAgentType::FixedWingUAV;
    
    StateTreeComponent = CreateDefaultSubobject<UMAStateTreeComponent>(TEXT("StateTreeComponent"));
    StateTreeComponent->SetStartLogicAutomatically(true);
    
    UCharacterMovementComponent* MovementComp = GetCharacterMovement();
    MovementComp->SetMovementMode(MOVE_Flying);
    MovementComp->DefaultLandMovementMode = MOVE_Flying;
    MovementComp->MaxFlySpeed = MaxSpeed;
    MovementComp->GravityScale = 0.f;
}

void AMAFixedWingUAVCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // 配置导航服务为固定翼飞行模式
    if (NavigationService)
    {
        NavigationService->bIsFlying = true;
        NavigationService->bIsFixedWing = true;
        NavigationService->MinFlightAltitude = CruiseAltitude;
    }
    
    if (SkillComponent)
    {
        SkillComponent->OnEnergyDepleted.AddDynamic(this, &AMAFixedWingUAVCharacter::OnEnergyDepleted);
        SkillComponent->EnergyDrainRate = 0.3f;
        SkillComponent->MaxEnergy = 150.f;
        SkillComponent->Energy = 150.f;
    }
    
    // 加载 StateTree 资产（仅当 StateTree 启用时）
    if (StateTreeComponent && StateTreeComponent->IsStateTreeEnabled() && !StateTreeComponent->HasStateTree())
    {
        UStateTree* ST = LoadObject<UStateTree>(nullptr, TEXT("/Game/StateTree/ST_FixedWingUAV.ST_FixedWingUAV"));
        if (ST)
        {
            StateTreeComponent->SetStateTreeAsset(ST);
        }
    }
    
    // 固定翼起始在空中
    FVector Loc = GetActorLocation();
    SetActorLocation(FVector(Loc.X, Loc.Y, CruiseAltitude));
}

void AMAFixedWingUAVCharacter::InitializeSkillSet()
{
    AvailableSkills.Add(EMASkillType::Navigate);
    AvailableSkills.Add(EMASkillType::Search);
    AvailableSkills.Add(EMASkillType::Charge);
}

void AMAFixedWingUAVCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // 固定翼始终消耗能量（必须保持飞行）
    if (ShouldDrainEnergy() && SkillComponent && SkillComponent->HasEnergy())
    {
        SkillComponent->DrainEnergy(DeltaTime);
    }
}

void AMAFixedWingUAVCharacter::OnEnergyDepleted()
{
    // 固定翼能量耗尽时取消导航
    if (NavigationService)
    {
        NavigationService->CancelNavigation();
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[FixedWingUAV %s] Energy depleted!"), *AgentLabel);
}
