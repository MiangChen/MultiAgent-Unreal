// MASubsystem.h
// Subsystem 统一访问层 - 类似 Python 的依赖注入容器
// 提供全局单点访问所有 Manager Subsystem

#pragma once

#include "CoreMinimal.h"

class UMAAgentManager;
class UMACommandManager;
class UMASelectionManager;
class UMASquadManager;
class UMAViewportManager;
class UMAEmergencyManager;
class UMAEditModeManager;
class UWorld;

/**
 * Subsystem 统一访问结构
 * 
 * 使用方式:
 *   FMASubsystem Subs = FMASubsystem::Get(GetWorld());
 *   Subs.CommandManager->SendCommand(...);
 * 
 * 或使用宏:
 *   MA_SUBS.CommandManager->SendCommand(...);
 */
struct MULTIAGENT_API FMASubsystem
{
    UMAAgentManager* AgentManager = nullptr;
    UMACommandManager* CommandManager = nullptr;
    UMASelectionManager* SelectionManager = nullptr;
    UMASquadManager* SquadManager = nullptr;
    UMAViewportManager* ViewportManager = nullptr;
    UMAEmergencyManager* EmergencyManager = nullptr;
    UMAEditModeManager* EditModeManager = nullptr;

    // 从 World 获取所有 Subsystem
    static FMASubsystem Get(UWorld* World);

    // 检查是否所有 Subsystem 都有效
    bool IsValid() const;
};

/**
 * 便捷宏 - 需要当前类有 GetWorld() 方法
 * 
 * 使用示例:
 *   MA_SUBS.CommandManager->SendCommandToAgent(Agent, EMACommand::Navigate);
 *   MA_SUBS.AgentManager->GetAllAgents();
 * 
 * 性能提示: 每次调用都会获取所有 Subsystem
 * 如果需要多次访问，建议先缓存:
 *   auto Subs = MA_SUBS;
 *   if (Subs.IsValid()) {
 *       Subs.CommandManager->SendCommand(...);
 *       Subs.AgentManager->GetAllAgents();
 *   }
 */
#define MA_SUBS FMASubsystem::Get(GetWorld())
