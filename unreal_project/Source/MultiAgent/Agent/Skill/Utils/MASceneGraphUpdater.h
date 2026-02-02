// MASceneGraphUpdater.h
// 场景图更新器 - 负责技能完成后更新场景图状态

#pragma once

#include "CoreMinimal.h"

class AMACharacter;
class UMASceneGraphManager;
enum class EMACommand : uint8;

/**
 * 场景图更新器
 * 
 * 职责:
 * - 技能完成后更新场景图中的节点状态
 * - 更新机器人位置
 * - 更新可拾取物品位置和携带状态
 * - 调用场景图管理器的实现方法
 * 
 * 设计原则:
 * - 单一职责：只负责场景图状态更新
 * - 与反馈生成器解耦：反馈生成器只负责生成反馈数据
 * - 统一入口：通过 UpdateAfterSkillCompletion 统一调用
 */
class MULTIAGENT_API FMASceneGraphUpdater
{
public:
    /**
     * 技能完成后更新场景图（统一入口）
     * 
     * @param Agent 执行技能的 Agent
     * @param Command 技能类型
     * @param bSuccess 执行是否成功
     */
    static void UpdateAfterSkillCompletion(AMACharacter* Agent, EMACommand Command, bool bSuccess);

private:
    /**
     * 获取场景图管理器
     * @param Agent Agent 指针
     * @return 场景图管理器指针，如果获取失败则返回 nullptr
     */
    static UMASceneGraphManager* GetSceneGraphManager(AMACharacter* Agent);
    
    /**
     * 更新机器人位置
     * @param Agent Agent 指针
     */
    static void UpdateRobotPosition(AMACharacter* Agent);
    
    /**
     * Navigate 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterNavigate(AMACharacter* Agent, bool bSuccess);
    
    /**
     * Search 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterSearch(AMACharacter* Agent, bool bSuccess);
    
    /**
     * Follow 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterFollow(AMACharacter* Agent, bool bSuccess);
    
    /**
     * Charge 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterCharge(AMACharacter* Agent, bool bSuccess);
    
    /**
     * Place 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterPlace(AMACharacter* Agent, bool bSuccess);
    
    /**
     * TakeOff 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterTakeOff(AMACharacter* Agent, bool bSuccess);
    
    /**
     * Land 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterLand(AMACharacter* Agent, bool bSuccess);
    
    /**
     * ReturnHome 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterReturnHome(AMACharacter* Agent, bool bSuccess);
    
    /**
     * HandleHazard 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterHandleHazard(AMACharacter* Agent, bool bSuccess);
    
    /**
     * Guide 技能完成后的场景图更新
     * @param Agent Agent 指针
     * @param bSuccess 是否成功
     */
    static void UpdateAfterGuide(AMACharacter* Agent, bool bSuccess);
};
