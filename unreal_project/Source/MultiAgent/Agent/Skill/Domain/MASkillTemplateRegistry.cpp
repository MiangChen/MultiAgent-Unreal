// MASkillTemplateRegistry.cpp

#include "MASkillTemplateRegistry.h"
#include "Core/Shared/Types/MATypes.h"

// Static member initialization
TMap<EMACommand, FMASkillCheckTemplate> FMASkillTemplateRegistry::BaseTemplates;
bool FMASkillTemplateRegistry::bInitialized = false;

void FMASkillTemplateRegistry::InitBaseTemplates()
{
	if (bInitialized) return;
	bInitialized = true;

	// Helper arrays
	const TArray<EMAConditionCheckItem> EnvItems = {
		EMAConditionCheckItem::LowVisibility,
		EMAConditionCheckItem::StrongWind
	};
	const TArray<EMAConditionCheckItem> RobotItems = {
		EMAConditionCheckItem::BatteryLow,
		EMAConditionCheckItem::RobotFault
	};
	const TArray<EMAConditionCheckItem> TargetItems = {
		EMAConditionCheckItem::TargetExists,
		EMAConditionCheckItem::TargetNearby
	};
	// Place技能的目标检查项
	// - TargetExists/TargetNearby: 检查被放置的物体 (object1)
	// - SurfaceTargetExists/SurfaceTargetNearby: 检查放置目标 (object2)
	//   在装货模式下，surface target就是承载者(carrier)
	const TArray<EMAConditionCheckItem> PlaceTargetItems = {
		EMAConditionCheckItem::TargetExists,
		EMAConditionCheckItem::TargetNearby,
		EMAConditionCheckItem::SurfaceTargetExists,
		EMAConditionCheckItem::SurfaceTargetNearby
	};

	// --- Navigate ---
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.PrecheckItems.Append(EnvItems);
		T.RuntimeCheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(EnvItems);
		BaseTemplates.Add(EMACommand::Navigate, T);
	}

	// --- Search ---
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.PrecheckItems.Append(EnvItems);
		T.RuntimeCheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(EnvItems);
		T.RuntimeCheckItems.Add(EMAConditionCheckItem::HighPriorityTargetDiscovery);
		BaseTemplates.Add(EMACommand::Search, T);
	}

	// --- Follow ---
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.PrecheckItems.Append(EnvItems);
		T.RuntimeCheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(EnvItems);
		BaseTemplates.Add(EMACommand::Follow, T);
	}

	// --- Charge ---
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.PrecheckItems.Append(EnvItems);
		T.RuntimeCheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(EnvItems);
		BaseTemplates.Add(EMACommand::Charge, T);
	}

	// --- TakePhoto ---
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.PrecheckItems.Append(EnvItems);
		T.PrecheckItems.Append(TargetItems);
		T.RuntimeCheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(EnvItems);
		BaseTemplates.Add(EMACommand::TakePhoto, T);
	}

	// --- Broadcast ---
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.PrecheckItems.Append(EnvItems);
		T.PrecheckItems.Append(TargetItems);
		T.RuntimeCheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(EnvItems);
		BaseTemplates.Add(EMACommand::Broadcast, T);
	}

	// --- HandleHazard ---
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.PrecheckItems.Append(EnvItems);
		T.PrecheckItems.Append(TargetItems);
		T.RuntimeCheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(EnvItems);
		BaseTemplates.Add(EMACommand::HandleHazard, T);
	}

	// --- Guide ---
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.PrecheckItems.Append(EnvItems);
		T.PrecheckItems.Append(TargetItems);
		T.RuntimeCheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(EnvItems);
		BaseTemplates.Add(EMACommand::Guide, T);
	}

	// --- Place ---
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.PrecheckItems.Append(EnvItems);
		T.PrecheckItems.Append(PlaceTargetItems);
		T.RuntimeCheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(EnvItems);
		BaseTemplates.Add(EMACommand::Place, T);
	}

	// --- TakeOff --- (no environment checks)
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(RobotItems);
		BaseTemplates.Add(EMACommand::TakeOff, T);
	}

	// --- Land --- (no environment checks)
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(RobotItems);
		BaseTemplates.Add(EMACommand::Land, T);
	}

	// --- ReturnHome --- (no environment checks)
	{
		FMASkillCheckTemplate T;
		T.PrecheckItems.Append(RobotItems);
		T.RuntimeCheckItems.Append(RobotItems);
		BaseTemplates.Add(EMACommand::ReturnHome, T);
	}

	// --- Idle --- (no checks)
	{
		FMASkillCheckTemplate T;
		BaseTemplates.Add(EMACommand::Idle, T);
	}

	// --- None --- (no checks)
	{
		FMASkillCheckTemplate T;
		BaseTemplates.Add(EMACommand::None, T);
	}
}

bool FMASkillTemplateRegistry::IsAerialRobot(EMAAgentType AgentType)
{
	return AgentType == EMAAgentType::UAV || AgentType == EMAAgentType::FixedWingUAV;
}

bool FMASkillTemplateRegistry::IsEnvironmentExemptSkill(EMACommand Command)
{
	return Command == EMACommand::TakeOff
		|| Command == EMACommand::Land
		|| Command == EMACommand::ReturnHome;
}

FMASkillCheckTemplate FMASkillTemplateRegistry::GetTemplate(EMACommand Command, EMAAgentType AgentType)
{
	InitBaseTemplates();

	const FMASkillCheckTemplate* Base = BaseTemplates.Find(Command);
	if (!Base)
	{
		// Unknown command - return empty template
		return FMASkillCheckTemplate();
	}

	// If aerial robot, return the base template as-is (env checks already included where appropriate)
	if (IsAerialRobot(AgentType))
	{
		return *Base;
	}

	// Ground robot: filter out environment check items
	FMASkillCheckTemplate Filtered;

	for (const EMAConditionCheckItem& Item : Base->PrecheckItems)
	{
		if (Item != EMAConditionCheckItem::LowVisibility && Item != EMAConditionCheckItem::StrongWind)
		{
			Filtered.PrecheckItems.Add(Item);
		}
	}

	for (const EMAConditionCheckItem& Item : Base->RuntimeCheckItems)
	{
		if (Item != EMAConditionCheckItem::LowVisibility && Item != EMAConditionCheckItem::StrongWind)
		{
			Filtered.RuntimeCheckItems.Add(Item);
		}
	}

	return Filtered;
}
