// MASkillParamsProcessor.h
// 技能参数处理器 - 统一处理各类技能的参数预处理

#pragma once

#include "CoreMinimal.h"

class AMACharacter;
class UMASkillComponent;
struct FMAAgentSkillCommand;
struct FMASemanticTarget;
enum class EMACommand : uint8;

/**
 * Place 技能操作模式
 */
enum class EPlaceMode : uint8
{
    LoadToUGV,       // 装货到 UGV
    UnloadToGround,  // 从 UGV 卸货到地面
    StackOnObject    // 堆叠到另一个物体
};

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
    
    // Place 技能辅助方法 (public for testing)
    /**
     * 从 JSON 字符串解析语义标签到 FMASemanticTarget
     */
    static void ParseSemanticTargetFromJson(const FString& JsonStr, FMASemanticTarget& OutTarget);
    
    /**
     * 根据 surface_target 语义标签确定 Place 操作模式
     */
    static EPlaceMode DeterminePlaceMode(const FMASemanticTarget& surface_target);

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
};
