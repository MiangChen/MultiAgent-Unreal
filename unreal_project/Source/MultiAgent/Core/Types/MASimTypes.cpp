// MASimTypes.cpp
// UI 集成相关的数据类型实现

#include "MASimTypes.h"
#include "../Agent/Character/MACharacter.h"

DEFINE_LOG_CATEGORY(LogMASimTypes);

//=============================================================================
// FMARobotData 实现
//=============================================================================

FMARobotData::FMARobotData(const AMACharacter* Character)
{
    if (Character)
    {
        RobotID = Character->AgentID;
        RobotName = Character->AgentLabel;
        RobotType = Character->AgentType;
        Location = Character->GetActorLocation();
        Rotation = Character->GetActorRotation();
        Battery = 1.0f; // TODO: 从 Character 获取实际电量
        CharacterRef = const_cast<AMACharacter*>(Character);
    }
}
