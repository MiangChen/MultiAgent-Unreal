// MASceneGraphUpdater.cpp
// 场景图更新器实现

#include "MASceneGraphUpdater.h"
#include "../../Character/MACharacter.h"
#include "../MASkillComponent.h"
#include "../../../Core/Manager/MACommandManager.h"
#include "../../../Core/Manager/MASceneGraphManager.h"
#include "../../../Core/Manager/scene_graph_tools/MASceneGraphQuery.h"

DEFINE_LOG_CATEGORY_STATIC(LogMASceneGraphUpdater, Log, All);

//=============================================================================
// 统一入口
//=============================================================================

void FMASceneGraphUpdater::UpdateAfterSkillCompletion(AMACharacter* Agent, EMACommand Command, bool bSuccess)
{
    if (!Agent)
    {
        return;
    }
    
    // 根据技能类型调用对应的更新方法
    switch (Command)
    {
        case EMACommand::Navigate:
            UpdateAfterNavigate(Agent, bSuccess);
            break;
        case EMACommand::Search:
            UpdateAfterSearch(Agent, bSuccess);
            break;
        case EMACommand::Follow:
            UpdateAfterFollow(Agent, bSuccess);
            break;
        case EMACommand::Charge:
            UpdateAfterCharge(Agent, bSuccess);
            break;
        case EMACommand::Place:
            UpdateAfterPlace(Agent, bSuccess);
            break;
        case EMACommand::TakeOff:
            UpdateAfterTakeOff(Agent, bSuccess);
            break;
        case EMACommand::Land:
            UpdateAfterLand(Agent, bSuccess);
            break;
        case EMACommand::ReturnHome:
            UpdateAfterReturnHome(Agent, bSuccess);
            break;
        default:
            // 其他技能不需要更新场景图
            break;
    }
}

//=============================================================================
// 辅助方法
//=============================================================================

UMASceneGraphManager* FMASceneGraphUpdater::GetSceneGraphManager(AMACharacter* Agent)
{
    if (!Agent)
    {
        return nullptr;
    }
    
    UWorld* World = Agent->GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }
    
    return GameInstance->GetSubsystem<UMASceneGraphManager>();
}

void FMASceneGraphUpdater::UpdateRobotPosition(AMACharacter* Agent)
{
    if (!Agent)
    {
        return;
    }
    
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (!SceneGraphManager)
    {
        UE_LOG(LogMASceneGraphUpdater, Warning, TEXT("Cannot update scene graph: SceneGraphManager not available"));
        return;
    }
    
    SceneGraphManager->UpdateRobotPosition(Agent->AgentID, Agent->GetActorLocation());
    UE_LOG(LogMASceneGraphUpdater, Verbose, TEXT("Updated robot '%s' position in scene graph to (%f, %f, %f)"),
        *Agent->AgentID, Agent->GetActorLocation().X, Agent->GetActorLocation().Y, Agent->GetActorLocation().Z);
}

//=============================================================================
// 各技能的场景图更新实现
//=============================================================================

void FMASceneGraphUpdater::UpdateAfterNavigate(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess)
    {
        return;
    }
    
    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterSearch(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess)
    {
        return;
    }
    
    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterFollow(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess)
    {
        return;
    }
    
    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterCharge(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess)
    {
        return;
    }
    
    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterPlace(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess || !Agent)
    {
        return;
    }
    
    UMASceneGraphManager* SceneGraphManager = GetSceneGraphManager(Agent);
    if (!SceneGraphManager)
    {
        UE_LOG(LogMASceneGraphUpdater, Warning, TEXT("Cannot update scene graph: SceneGraphManager not available"));
        return;
    }
    
    // 更新机器人位置
    UpdateRobotPosition(Agent);
    
    // 获取技能组件和反馈上下文
    UMASkillComponent* SkillComp = Agent->GetSkillComponent();
    if (!SkillComp)
    {
        return;
    }
    
    const FMAFeedbackContext& Context = SkillComp->GetFeedbackContext();
    
    // 获取物品和目标节点ID
    FString Object1NodeId = Context.ObjectAttributes.FindRef(TEXT("object1_node_id"));
    FString Object2NodeId = Context.ObjectAttributes.FindRef(TEXT("object2_node_id"));
    
    if (Object1NodeId.IsEmpty() || Context.PlaceFinalLocation.IsZero())
    {
        return;
    }
    
    // 更新物品位置
    SceneGraphManager->UpdatePickupItemPosition(Object1NodeId, Context.PlaceFinalLocation);
    
    // 判断是否放到了机器人上
    bool bPlacedOnRobot = false;
    FString CarrierId;
    
    if (!Object2NodeId.IsEmpty())
    {
        TArray<FMASceneGraphNode> AllNodes = SceneGraphManager->GetAllNodes();
        FMASceneGraphNode Object2Node = FMASceneGraphQuery::FindNodeByIdOrLabel(AllNodes, Object2NodeId);
        
        if (Object2Node.IsValid() && Object2Node.IsRobot())
        {
            bPlacedOnRobot = true;
            CarrierId = Object2NodeId;
        }
    }
    
    // 更新携带状态
    SceneGraphManager->UpdatePickupItemCarrierStatus(Object1NodeId, bPlacedOnRobot, CarrierId);
    
    UE_LOG(LogMASceneGraphUpdater, Log, TEXT("Updated scene graph after Place: Item=%s, Location=(%f, %f, %f), Carried=%s, Carrier=%s"),
        *Object1NodeId, Context.PlaceFinalLocation.X, Context.PlaceFinalLocation.Y, Context.PlaceFinalLocation.Z,
        bPlacedOnRobot ? TEXT("true") : TEXT("false"), *CarrierId);
}

void FMASceneGraphUpdater::UpdateAfterTakeOff(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess)
    {
        return;
    }
    
    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterLand(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess)
    {
        return;
    }
    
    UpdateRobotPosition(Agent);
}

void FMASceneGraphUpdater::UpdateAfterReturnHome(AMACharacter* Agent, bool bSuccess)
{
    if (!bSuccess)
    {
        return;
    }
    
    UpdateRobotPosition(Agent);
}
