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

    // 从 World 获取所有 Subsystem
    static FMASubsystem Get(UWorld* World);

    // 检查是否所有 Subsystem 都有效
    bool IsValid() const;
};

/**
 * 便捷宏 - 需要当前类有 GetWorld() 方法
 * 
 * 使用示例:
 *   MA_SUBS.CommandManager->SendCommand(EMACommand::Patrol);
 *   MA_SUBS.AgentManager->GetAllAgents();
 */
#define MA_SUBS FMASubsystem::Get(GetWorld())
