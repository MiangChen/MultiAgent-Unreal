// MASensorAgent.cpp

#include "MASensorAgent.h"
#include "Components/CapsuleComponent.h"

AMASensorAgent::AMASensorAgent()
{
    // Sensor 默认不需要 AI Controller
    AIControllerClass = nullptr;
    AutoPossessAI = EAutoPossessAI::Disabled;
    
    // 禁用碰撞（Sensor 不需要物理碰撞）
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMASensorAgent::AttachToAgent(AMAAgent* ParentAgent, FVector RelativeLocation, FRotator RelativeRotation)
{
    if (!ParentAgent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Sensor] AttachToAgent failed: ParentAgent is null"));
        return;
    }
    
    AttachedToAgent = ParentAgent;
    
    // 附着到父 Agent
    FAttachmentTransformRules Rules(
        EAttachmentRule::KeepRelative,
        EAttachmentRule::KeepRelative,
        EAttachmentRule::KeepWorld,
        true
    );
    
    AttachToActor(ParentAgent, Rules);
    SetActorRelativeLocation(RelativeLocation);
    SetActorRelativeRotation(RelativeRotation);
    
    UE_LOG(LogTemp, Log, TEXT("[Sensor] %s attached to %s at (%.0f, %.0f, %.0f)"),
        *AgentName, *ParentAgent->AgentName,
        RelativeLocation.X, RelativeLocation.Y, RelativeLocation.Z);
}

void AMASensorAgent::DetachFromAgent()
{
    if (!AttachedToAgent.IsValid())
    {
        return;
    }
    
    FString ParentName = AttachedToAgent->AgentName;
    
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    AttachedToAgent.Reset();
    
    UE_LOG(LogTemp, Log, TEXT("[Sensor] %s detached from %s"), *AgentName, *ParentName);
}

AMAAgent* AMASensorAgent::GetAttachedAgent() const
{
    return AttachedToAgent.Get();
}

bool AMASensorAgent::IsAttached() const
{
    return AttachedToAgent.IsValid();
}
