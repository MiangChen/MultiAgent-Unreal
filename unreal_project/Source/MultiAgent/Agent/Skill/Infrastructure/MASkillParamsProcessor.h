// MASkillParamsProcessor.h
// 技能参数处理器 - 统一处理各类技能的参数预处理

#pragma once

#include "CoreMinimal.h"
#include "Core/Command/Domain/MACommandTypes.h"

class AMACharacter;
class UMASkillComponent;
struct FMAAgentSkillCommand;

/**
 * 技能参数处理器
 * 
 * 职责:
 * - 统一的参数预处理接口
 * - 根据技能类型分发到具体处理函数
 * - 执行场景查询等预处理操作
 */
class MULTIAGENT_API FMASkillParamsProcessor
{
public:
    /**
     * 处理技能参数（统一入口）
     * @param Agent 目标 Agent
     * @param Command 指令类型
     * @param Cmd 原始指令数据（可选，用于从通信层传入的参数）
     */
    static void Process(AMACharacter* Agent, EMACommand Command, const FMAAgentSkillCommand* Cmd = nullptr);

private:
    // 各技能的参数处理
    static void ProcessNavigate(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessSearch(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessFollow(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessCharge(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessPlace(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessTakeOff(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessLand(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessReturnHome(UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    
    // 新技能的参数处理
    static void ProcessTakePhoto(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessBroadcast(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessHandleHazard(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessGuide(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
};
