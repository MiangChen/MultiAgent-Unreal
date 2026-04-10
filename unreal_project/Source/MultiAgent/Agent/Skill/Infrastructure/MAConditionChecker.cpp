// MAConditionChecker.cpp

#include "MAConditionChecker.h"
#include "Agent/Skill/Domain/MASkillTemplateRegistry.h"
#include "Agent/Skill/Domain/MAEventTemplateRegistry.h"
#include "Agent/Skill/Infrastructure/MASkillSceneGraphBridge.h"
#include "Core/Config/MAConfigManager.h"
#include "Core/Shared/Types/MATypes.h"
#include "Agent/CharacterRuntime/Runtime/MACharacter.h"
#include "Agent/Skill/Runtime/MASkillComponent.h"

namespace
{
struct FMAConditionCheckContext
{
	AMACharacter* Agent = nullptr;
	UMASkillComponent* SkillComp = nullptr;
	FVector AgentLocation = FVector::ZeroVector;

	explicit FMAConditionCheckContext(AMACharacter* InAgent, UMASkillComponent* InSkillComp)
		: Agent(InAgent)
		, SkillComp(InSkillComp)
		, AgentLocation(InAgent ? InAgent->GetActorLocation() : FVector::ZeroVector)
	{
	}

	const TArray<FMASceneGraphNode>& GetAllNodes() const
	{
		if (!bAllNodesLoaded)
		{
			AllNodes = Agent ? FMASkillSceneGraphBridge::LoadAllNodes(Agent) : TArray<FMASceneGraphNode>();
			bAllNodesLoaded = true;
			for (const FMASceneGraphNode& Node : AllNodes)
			{
				NodeCache.FindOrAdd(Node.Id) = Node;
			}
		}

		return AllNodes;
	}

	FMASceneGraphNode GetNodeById(const FString& NodeId) const
	{
		if (!Agent || NodeId.IsEmpty())
		{
			return FMASceneGraphNode();
		}

		if (const FMASceneGraphNode* CachedNode = NodeCache.Find(NodeId))
		{
			return *CachedNode;
		}

		const FMASceneGraphNode Node = FMASkillSceneGraphBridge::LoadNodeById(Agent, NodeId);
		if (Node.IsValid())
		{
			NodeCache.Add(NodeId, Node);
		}
		return Node;
	}

	bool ContainsNodeId(const FString& NodeId) const
	{
		if (!Agent || NodeId.IsEmpty())
		{
			return false;
		}

		if (NodeCache.Contains(NodeId))
		{
			return true;
		}

		if (const bool* CachedExists = NodeExistsCache.Find(NodeId))
		{
			return *CachedExists;
		}

		const bool bExists = FMASkillSceneGraphBridge::ContainsNodeId(Agent, NodeId);
		NodeExistsCache.Add(NodeId, bExists);
		return bExists;
	}

private:
	mutable bool bAllNodesLoaded = false;
	mutable TArray<FMASceneGraphNode> AllNodes;
	mutable TMap<FString, FMASceneGraphNode> NodeCache;
	mutable TMap<FString, bool> NodeExistsCache;
};

FMACheckResult CheckEnvironmentHazardWithContext(
	const FMAConditionCheckContext& Context,
	const FString& NodeType,
	const float DefaultRadius,
	const FString& EventKey)
{
	if (!Context.Agent)
	{
		return FMACheckResult::Pass();
	}

	const TArray<FMASceneGraphNode>& AllNodes = Context.GetAllNodes();
	if (AllNodes.Num() == 0)
	{
		return FMACheckResult::Pass();
	}

	for (const FMASceneGraphNode& Node : AllNodes)
	{
		if (!Node.Type.Equals(NodeType, ESearchCase::IgnoreCase))
		{
			continue;
		}

		float EffectRadius = DefaultRadius;
		if (const FString* RadiusStr = Node.Features.Find(TEXT("effect_radius")))
		{
			const float Parsed = FCString::Atof(**RadiusStr);
			if (Parsed > 0.f)
			{
				EffectRadius = Parsed;
			}
		}

		if (FVector::Dist(Context.AgentLocation, Node.Center) <= EffectRadius)
		{
			TMap<FString, FString> Params;
			Params.Add(TEXT("robot_label"), Context.Agent->AgentLabel);
			Params.Add(TEXT("node_id"), Node.Id);
			Params.Add(TEXT("node_label"), Node.GetDisplayName());
			return FMACheckResult::Fail(EventKey, Params);
		}
	}

	return FMACheckResult::Pass();
}

FMACheckResult CheckObjectExistsWithContext(
	const FMAConditionCheckContext& Context,
	const FString& ObjectId,
	const FString& EventKey,
	const FString& LabelParamKey)
{
	if (ObjectId.IsEmpty() || !Context.Agent)
	{
		return FMACheckResult::Pass();
	}

	if (Context.GetNodeById(ObjectId).IsValid() || Context.ContainsNodeId(ObjectId))
	{
		return FMACheckResult::Pass();
	}

	TMap<FString, FString> Params;
	Params.Add(LabelParamKey, ObjectId);
	return FMACheckResult::Fail(EventKey, Params);
}

FMACheckResult CheckObjectNearbyWithContext(
	const FMAConditionCheckContext& Context,
	const FString& ObjectId,
	const FString& EventKey,
	const FString& LabelParamKey)
{
	if (!Context.Agent || ObjectId.IsEmpty())
	{
		return FMACheckResult::Pass();
	}

	const FMASceneGraphNode Node = Context.GetNodeById(ObjectId);
	if (!Node.IsValid())
	{
		return FMACheckResult::Pass();
	}

	const float Distance = FVector::Dist(Context.AgentLocation, Node.Center);
	if (Distance <= FMAConditionChecker::DistanceThresholdCm)
	{
		return FMACheckResult::Pass();
	}

	TMap<FString, FString> Params;
	Params.Add(LabelParamKey, Node.GetDisplayName());
	Params.Add(TEXT("robot_label"), Context.Agent->AgentLabel);
	Params.Add(TEXT("distance"), FString::Printf(TEXT("%.1f"), Distance / 100.f));
	Params.Add(TEXT("threshold"), FString::Printf(TEXT("%.1f"), FMAConditionChecker::DistanceThresholdCm / 100.f));
	return FMACheckResult::Fail(EventKey, Params);
}

FMACheckResult CheckHighPriorityTargetDiscoveryWithContext(const FMAConditionCheckContext& Context)
{
	if (!Context.Agent || !Context.SkillComp)
	{
		return FMACheckResult::Pass();
	}

	const FMASemanticTarget& SearchTarget = Context.SkillComp->GetSkillParams().SearchTarget;
	if (!SearchTarget.IsEmpty())
	{
		bool bTargetIsHighPriority = false;

		if (SearchTarget.Type.Equals(TEXT("fire"), ESearchCase::IgnoreCase) ||
			SearchTarget.Type.Equals(TEXT("hazard"), ESearchCase::IgnoreCase))
		{
			bTargetIsHighPriority = true;
		}

		const FString* IsFireVal = SearchTarget.Features.Find(TEXT("is_fire"));
		const FString* IsSpillVal = SearchTarget.Features.Find(TEXT("is_spill"));
		if ((IsFireVal && IsFireVal->Equals(TEXT("true"), ESearchCase::IgnoreCase)) ||
			(IsSpillVal && IsSpillVal->Equals(TEXT("true"), ESearchCase::IgnoreCase)))
		{
			bTargetIsHighPriority = true;
		}

		if (bTargetIsHighPriority)
		{
			return FMACheckResult::Pass();
		}
	}

	const TArray<FMASceneGraphNode>& AllNodes = Context.GetAllNodes();
	if (AllNodes.Num() == 0)
	{
		return FMACheckResult::Pass();
	}

	static const float ScanRadius = 5000.f;
	for (const FMASceneGraphNode& Node : AllNodes)
	{
		if (FVector::Dist2D(Context.AgentLocation, Node.Center) > ScanRadius)
		{
			continue;
		}

		bool bIsHighPriority = false;
		FString HazardType;
		if (Node.Type.Equals(TEXT("fire"), ESearchCase::IgnoreCase))
		{
			bIsHighPriority = true;
			HazardType = TEXT("fire");
		}
		else if (Node.Type.Equals(TEXT("hazard"), ESearchCase::IgnoreCase))
		{
			bIsHighPriority = true;
			HazardType = TEXT("hazard");
		}
		else
		{
			const FString* IsFireStr = Node.Features.Find(TEXT("is_fire"));
			const FString* IsSpillStr = Node.Features.Find(TEXT("is_spill"));
			if (IsFireStr && IsFireStr->Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				bIsHighPriority = true;
				HazardType = TEXT("fire");
			}
			else if (IsSpillStr && IsSpillStr->Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				bIsHighPriority = true;
				HazardType = TEXT("hazard");
			}
		}

		if (!bIsHighPriority)
		{
			continue;
		}

		TMap<FString, FString> Params;
		Params.Add(TEXT("hp_target_label"), Node.GetDisplayName());
		Params.Add(TEXT("hp_target_id"), Node.Id);
		Params.Add(TEXT("hp_target_type"), HazardType);
		return FMACheckResult::Fail(TEXT("NEW_HIGH_PRIORITY_TARGET_DISCOVERED"), Params);
	}

	return FMACheckResult::Pass();
}

FMACheckResult CheckRobotFaultWithContext(const FMAConditionCheckContext& Context)
{
	if (!Context.Agent)
	{
		return FMACheckResult::Pass();
	}

	if (!IsValid(Context.Agent) || Context.Agent->IsPendingKillPending())
	{
		TMap<FString, FString> Params;
		Params.Add(TEXT("robot_label"), Context.Agent->AgentLabel);
		return FMACheckResult::Fail(TEXT("ROBOT_FAULT"), Params);
	}

	const FMASceneGraphNode Node = Context.GetNodeById(Context.Agent->AgentID);
	if (Node.IsValid())
	{
		const FString* StatusStr = Node.Features.Find(TEXT("status"));
		if (StatusStr && StatusStr->Equals(TEXT("error"), ESearchCase::IgnoreCase))
		{
			TMap<FString, FString> Params;
			Params.Add(TEXT("robot_label"), Context.Agent->AgentLabel);
			return FMACheckResult::Fail(TEXT("ROBOT_FAULT"), Params);
		}
	}

	return FMACheckResult::Pass();
}

FMACheckResult ExecuteCheckItemWithContext(
	const EMAConditionCheckItem Item,
	const FMAConditionCheckContext& Context,
	const EMACommand Command)
{
	switch (Item)
	{
	case EMAConditionCheckItem::LowVisibility:
		return CheckEnvironmentHazardWithContext(Context, TEXT("smoke"), 3000.f, TEXT("AREA_LOW_VISIBILITY"));
	case EMAConditionCheckItem::StrongWind:
		return CheckEnvironmentHazardWithContext(Context, TEXT("wind"), 5000.f, TEXT("AREA_STRONG_WIND"));
	case EMAConditionCheckItem::TargetExists:
		return CheckObjectExistsWithContext(Context, FMAConditionChecker::GetTargetObjectId(Command, Context.SkillComp), TEXT("TARGET_NOT_EXIST"), TEXT("target_label"));
	case EMAConditionCheckItem::TargetNearby:
		return CheckObjectNearbyWithContext(Context, FMAConditionChecker::GetTargetObjectId(Command, Context.SkillComp), TEXT("TARGET_LOCATION_MISMATCH"), TEXT("target_label"));
	case EMAConditionCheckItem::SurfaceTargetExists:
		if (!Context.SkillComp) return FMACheckResult::Pass();
		{
			const FString* SurfaceId = Context.SkillComp->GetFeedbackContext().ObjectAttributes.Find(TEXT("object2_node_id"));
			return (!SurfaceId || SurfaceId->IsEmpty())
				? FMACheckResult::Pass()
				: CheckObjectExistsWithContext(Context, *SurfaceId, TEXT("SURFACE_OBJECT_MISSING"), TEXT("surface_label"));
		}
	case EMAConditionCheckItem::SurfaceTargetNearby:
		if (!Context.SkillComp) return FMACheckResult::Pass();
		{
			const FString* SurfaceId = Context.SkillComp->GetFeedbackContext().ObjectAttributes.Find(TEXT("object2_node_id"));
			return (!SurfaceId || SurfaceId->IsEmpty())
				? FMACheckResult::Pass()
				: CheckObjectNearbyWithContext(Context, *SurfaceId, TEXT("SURFACE_OBJECT_LOCATION_MISMATCH"), TEXT("surface_label"));
		}
	case EMAConditionCheckItem::HighPriorityTargetDiscovery:
		return CheckHighPriorityTargetDiscoveryWithContext(Context);
	case EMAConditionCheckItem::BatteryLow:
		return FMAConditionChecker::CheckBatteryLow(Context.Agent, Context.SkillComp);
	case EMAConditionCheckItem::RobotFault:
		return CheckRobotFaultWithContext(Context);
	default:
		return FMACheckResult::Pass();
	}
}
}

//=============================================================================
// 辅助方法
//=============================================================================

bool FMAConditionChecker::IsAerialRobot(EMAAgentType AgentType)
{
	return FMASkillTemplateRegistry::IsAerialRobot(AgentType);
}

bool FMAConditionChecker::IsEnvironmentExemptSkill(EMACommand Command)
{
	return FMASkillTemplateRegistry::IsEnvironmentExemptSkill(Command);
}

FString FMAConditionChecker::GetTargetObjectId(EMACommand Command, UMASkillComponent* SkillComp)
{
	if (!SkillComp) return FString();

	const FMASkillParams& Params = SkillComp->GetSkillParams();

	switch (Command)
	{
	case EMACommand::Guide:
		return Params.GuideTargetObjectId;

	case EMACommand::TakePhoto:
	case EMACommand::Broadcast:
	case EMACommand::HandleHazard:
		return Params.CommonTargetObjectId;

	case EMACommand::Place:
		// For Place, the primary target is PlaceObject1 (the cargo).
		// We look for a matching node by checking SearchResults.
		// The object1 node ID is stored in FeedbackContext during param processing.
		{
			const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
			const FString* NodeId = Ctx.ObjectAttributes.Find(TEXT("object1_node_id"));
			return NodeId ? *NodeId : FString();
		}

	default:
		return FString();
	}
}

//=============================================================================
// 环境检查
//=============================================================================

FMACheckResult FMAConditionChecker::CheckEnvironmentHazard(
	AMACharacter* Agent,
	const FString& NodeType,
	float DefaultRadius,
	const FString& EventKey)
{
	return CheckEnvironmentHazardWithContext(FMAConditionCheckContext(Agent, nullptr), NodeType, DefaultRadius, EventKey);
}

FMACheckResult FMAConditionChecker::CheckLowVisibility(AMACharacter* Agent)
{
	return CheckEnvironmentHazard(Agent, TEXT("smoke"), 3000.f, TEXT("AREA_LOW_VISIBILITY"));
}

FMACheckResult FMAConditionChecker::CheckStrongWind(AMACharacter* Agent)
{
	return CheckEnvironmentHazard(Agent, TEXT("wind"), 5000.f, TEXT("AREA_STRONG_WIND"));
}

//=============================================================================
// 目标检查 - 通用辅助
//=============================================================================

FMACheckResult FMAConditionChecker::CheckObjectExists(
	AMACharacter* Agent,
	const FString& ObjectId,
	const FString& EventKey,
	const FString& LabelParamKey)
{
	return CheckObjectExistsWithContext(FMAConditionCheckContext(Agent, nullptr), ObjectId, EventKey, LabelParamKey);
}

FMACheckResult FMAConditionChecker::CheckObjectNearby(
	AMACharacter* Agent,
	const FString& ObjectId,
	const FString& EventKey,
	const FString& LabelParamKey)
{
	return CheckObjectNearbyWithContext(FMAConditionCheckContext(Agent, nullptr), ObjectId, EventKey, LabelParamKey);
}

//=============================================================================
// 目标检查 - 具体实现
//=============================================================================

FMACheckResult FMAConditionChecker::CheckTargetExists(
	AMACharacter* Agent, UMASkillComponent* SkillComp, EMACommand Command)
{
	const FString TargetId = GetTargetObjectId(Command, SkillComp);
	return CheckObjectExists(Agent, TargetId, TEXT("TARGET_NOT_EXIST"), TEXT("target_label"));
}

FMACheckResult FMAConditionChecker::CheckTargetNearby(
	AMACharacter* Agent, UMASkillComponent* SkillComp, EMACommand Command)
{
	const FString TargetId = GetTargetObjectId(Command, SkillComp);
	return CheckObjectNearby(Agent, TargetId, TEXT("TARGET_LOCATION_MISMATCH"), TEXT("target_label"));
}

FMACheckResult FMAConditionChecker::CheckSurfaceTargetExists(
	AMACharacter* Agent, UMASkillComponent* SkillComp)
{
	if (!SkillComp) return FMACheckResult::Pass();

	// Surface target is PlaceObject2 - check if we have a resolved node ID
	const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
	const FString* SurfaceId = Ctx.ObjectAttributes.Find(TEXT("object2_node_id"));

	// If no surface ID stored, the surface might be "ground" which doesn't need existence check
	if (!SurfaceId || SurfaceId->IsEmpty())
	{
		return FMACheckResult::Pass();
	}

	return CheckObjectExists(Agent, *SurfaceId, TEXT("SURFACE_OBJECT_MISSING"), TEXT("surface_label"));
}

FMACheckResult FMAConditionChecker::CheckSurfaceTargetNearby(
	AMACharacter* Agent, UMASkillComponent* SkillComp)
{
	if (!SkillComp) return FMACheckResult::Pass();

	const FMAFeedbackContext& Ctx = SkillComp->GetFeedbackContext();
	const FString* SurfaceId = Ctx.ObjectAttributes.Find(TEXT("object2_node_id"));
	if (!SurfaceId || SurfaceId->IsEmpty())
	{
		return FMACheckResult::Pass();
	}

	return CheckObjectNearby(Agent, *SurfaceId, TEXT("SURFACE_OBJECT_LOCATION_MISMATCH"), TEXT("surface_label"));
}

FMACheckResult FMAConditionChecker::CheckHighPriorityTargetDiscovery(
	AMACharacter* Agent, UMASkillComponent* SkillComp)
{
	return CheckHighPriorityTargetDiscoveryWithContext(FMAConditionCheckContext(Agent, SkillComp));
}

//=============================================================================
// 机器人检查
//=============================================================================

FMACheckResult FMAConditionChecker::CheckBatteryLow(AMACharacter* Agent, UMASkillComponent* SkillComp)
{
	if (!Agent || !SkillComp)
	{
		return FMACheckResult::Pass();
	}

	const float EnergyPercent = SkillComp->GetEnergyPercent();
	if (EnergyPercent >= BatteryMinPercent)
	{
		return FMACheckResult::Pass();
	}

	TMap<FString, FString> Params;
	Params.Add(TEXT("robot_label"), Agent->AgentLabel);
	Params.Add(TEXT("battery_level"), FString::Printf(TEXT("%.0f"), EnergyPercent));
	return FMACheckResult::Fail(TEXT("ROBOT_BATTERY_LOW"), Params);
}

FMACheckResult FMAConditionChecker::CheckRobotFault(AMACharacter* Agent)
{
	return CheckRobotFaultWithContext(FMAConditionCheckContext(Agent, nullptr));
}

//=============================================================================
// 单项检查分发
//=============================================================================

FMACheckResult FMAConditionChecker::ExecuteCheckItem(
	EMAConditionCheckItem Item,
	AMACharacter* Agent,
	EMACommand Command,
	UMASkillComponent* SkillComp)
{
	return ExecuteCheckItemWithContext(Item, FMAConditionCheckContext(Agent, SkillComp), Command);
}

//=============================================================================
// 聚合逻辑
//=============================================================================

FMAPrecheckResult FMAConditionChecker::RunChecks(
	const TArray<EMAConditionCheckItem>& Items,
	AMACharacter* Agent,
	EMACommand Command,
	UMASkillComponent* SkillComp)
{
	FMAPrecheckResult Result;
	const FMAConditionCheckContext Context(Agent, SkillComp);
	bool bEnableInfoChecks = true;

	if (Agent)
	{
		if (UGameInstance* GameInstance = Agent->GetGameInstance())
		{
			if (UMAConfigManager* ConfigMgr = GameInstance->GetSubsystem<UMAConfigManager>())
			{
				bEnableInfoChecks = ConfigMgr->bEnableInfoChecks;
			}
		}
	}

	// 读取 Info 级别检查开关
	bool bEnableInfoChecks = true;
	if (Agent)
	{
		if (UGameInstance* GI = Agent->GetGameInstance())
		{
			if (UMAConfigManager* ConfigMgr = GI->GetSubsystem<UMAConfigManager>())
			{
				bEnableInfoChecks = ConfigMgr->bEnableInfoChecks;
			}
		}
	}

	for (const EMAConditionCheckItem& Item : Items)
	{
		FMACheckResult CheckResult = ExecuteCheckItemWithContext(Item, Context, Command);

		if (CheckResult.bPassed)
		{
			continue;
		}

		// Render the event from the template
		FMARenderedEvent Event = FMAEventTemplateRegistry::RenderEvent(CheckResult.EventKey, CheckResult.EventParams);

		// Store the event key in the payload for downstream consumers
		Event.Payload.Add(TEXT("event_key"), CheckResult.EventKey);

		if (Event.IsAbort())
		{
			Result.bAllPassed = false;
			Result.FailedEvents.Add(Event);
		}
		else if (bEnableInfoChecks)
		{
			// Info-level event (only collected when enabled by config)
			Result.InfoEvents.Add(Event);
		}
	}

	return Result;
}

FMAPrecheckResult FMAConditionChecker::RunPrecheck(
	AMACharacter* Agent,
	EMACommand Command,
	UMASkillComponent* SkillComp)
{
	// Boundary: if Agent or SkillComponent is invalid, pass (don't block)
	if (!Agent || !SkillComp)
	{
		return FMAPrecheckResult();
	}

	const FMASkillCheckTemplate Template = FMASkillTemplateRegistry::GetTemplate(Command, Agent->AgentType);
	return RunChecks(Template.PrecheckItems, Agent, Command, SkillComp);
}

FMAPrecheckResult FMAConditionChecker::RunRuntimeCheck(
	AMACharacter* Agent,
	EMACommand Command,
	UMASkillComponent* SkillComp)
{
	if (!Agent || !SkillComp)
	{
		return FMAPrecheckResult();
	}

	const FMASkillCheckTemplate Template = FMASkillTemplateRegistry::GetTemplate(Command, Agent->AgentType);
	return RunChecks(Template.RuntimeCheckItems, Agent, Command, SkillComp);
}
