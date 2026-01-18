// MASkillParamsProcessor.h
// 技能参数处理器 - 统一处理各类技能的参数预处理

#pragma once

#include "CoreMinimal.h"

class AMACharacter;
class UMASkillComponent;
struct FMAAgentSkillCommand;
struct FMASemanticTarget;
enum class EMACommand : uint8;
enum class EPlaceMode : uint8;  // Forward declaration - defined in SK_Place.h

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
    
    // 新技能的参数处理
    static void ProcessTakePhoto(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessBroadcast(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessHandleHazard(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    static void ProcessGuide(AMACharacter* Agent, UMASkillComponent* SkillComp, const FMAAgentSkillCommand* Cmd);
    
    /**
     * 通用对象匹配辅助函数
     * 
     * 匹配策略:
     * 1. 如果 ObjectId 非空，直接使用 object_id 在场景图中查找
     * 2. 如果 ObjectId 为空，使用语义标签在指定半径内匹配
     * 3. 如果场景图查询失败，回退到 UE5 场景查询
     * 
     * @param Agent 执行技能的 Agent
     * @param ObjectId 目标对象 ID（优先使用）
     * @param SemanticTarget 语义标签（当 ObjectId 为空时使用）
     * @param SearchRadius 搜索半径（单位：cm）
     * @param OutFoundId 输出：找到的对象 ID
     * @param OutFoundName 输出：找到的对象名称
     * @param OutFoundLocation 输出：找到的对象位置
     * @return 是否找到目标对象
     */
    static bool MatchTargetObject(
        AMACharacter* Agent,
        const FString& ObjectId,
        const FMASemanticTarget& SemanticTarget,
        float SearchRadius,
        FString& OutFoundId,
        FString& OutFoundName,
        FVector& OutFoundLocation);
};
