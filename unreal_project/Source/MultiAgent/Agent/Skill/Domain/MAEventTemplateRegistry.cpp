// MAEventTemplateRegistry.cpp

#include "MAEventTemplateRegistry.h"

// Static member initialization
TMap<FString, FMAEventTemplate> FMAEventTemplateRegistry::Templates;
bool FMAEventTemplateRegistry::bInitialized = false;

void FMAEventTemplateRegistry::InitTemplates()
{
	if (bInitialized) return;
	bInitialized = true;

	// --- Environment events ---

	{
		FMAEventTemplate T;
		T.Category = TEXT("environment");
		T.Type = TEXT("perception_degraded");
		T.Severity = EMAEventSeverity::SoftAbort;
		T.MessageTemplate = TEXT("{robot_label}'s current area is covered by smoke with low visibility.");
		Templates.Add(TEXT("AREA_LOW_VISIBILITY"), T);
	}

	{
		FMAEventTemplate T;
		T.Category = TEXT("environment");
		T.Type = TEXT("condition_hazard");
		T.Severity = EMAEventSeverity::Abort;
		T.MessageTemplate = TEXT("Strong wind detected near {robot_label}. Flying operations are unsafe.");
		Templates.Add(TEXT("AREA_STRONG_WIND"), T);
	}

	// --- Target events ---

	{
		FMAEventTemplate T;
		T.Category = TEXT("target");
		T.Type = TEXT("target_missing");
		T.Severity = EMAEventSeverity::Abort;
		T.MessageTemplate = TEXT("Target '{target_label}' could not be found.");
		Templates.Add(TEXT("TARGET_NOT_EXIST"), T);
	}

	{
		FMAEventTemplate T;
		T.Category = TEXT("target");
		T.Type = TEXT("location_mismatch");
		T.Severity = EMAEventSeverity::Abort;
		T.MessageTemplate = TEXT("Target '{target_label}' could not be found at {robot_label}'s current location.");
		Templates.Add(TEXT("TARGET_LOCATION_MISMATCH"), T);
	}

	{
		FMAEventTemplate T;
		T.Category = TEXT("target");
		T.Type = TEXT("resource_missing");
		T.Severity = EMAEventSeverity::Abort;
		T.MessageTemplate = TEXT("Carrier '{carrier_label}' not found.");
		Templates.Add(TEXT("CARRIER_NOT_FOUND"), T);
	}

	{
		FMAEventTemplate T;
		T.Category = TEXT("target");
		T.Type = TEXT("location_mismatch");
		T.Severity = EMAEventSeverity::Abort;
		T.MessageTemplate = TEXT("Carrier '{carrier_label}' is too far from {robot_label}.");
		Templates.Add(TEXT("CARRIER_LOCATION_MISMATCH"), T);
	}

	{
		FMAEventTemplate T;
		T.Category = TEXT("target");
		T.Type = TEXT("resource_missing");
		T.Severity = EMAEventSeverity::Abort;
		T.MessageTemplate = TEXT("Placement surface '{surface_label}' could not be found.");
		Templates.Add(TEXT("SURFACE_OBJECT_MISSING"), T);
	}

	{
		FMAEventTemplate T;
		T.Category = TEXT("target");
		T.Type = TEXT("location_mismatch");
		T.Severity = EMAEventSeverity::Abort;
		T.MessageTemplate = TEXT("Placement surface '{surface_label}' could not be found at {robot_label}'s current location.");
		Templates.Add(TEXT("SURFACE_OBJECT_LOCATION_MISMATCH"), T);
	}

	{
		FMAEventTemplate T;
		T.Category = TEXT("target");
		T.Type = TEXT("new_target_discovered");
		T.Severity = EMAEventSeverity::Info;
		T.MessageTemplate = TEXT("New high-priority target '{hp_target_label}' (with {hp_target_type}) discovered.");
		Templates.Add(TEXT("NEW_HIGH_PRIORITY_TARGET_DISCOVERED"), T);
	}

	// --- Robot events ---

	{
		FMAEventTemplate T;
		T.Category = TEXT("robot");
		T.Type = TEXT("capability_degraded");
		T.Severity = EMAEventSeverity::SoftAbort;
		T.MessageTemplate = TEXT("Robot '{robot_label}' battery level ({battery_level}%) is below minimum threshold.");
		Templates.Add(TEXT("ROBOT_BATTERY_LOW"), T);
	}

	{
		FMAEventTemplate T;
		T.Category = TEXT("robot");
		T.Type = TEXT("critical_failure");
		T.Severity = EMAEventSeverity::Abort;
		T.MessageTemplate = TEXT("Robot '{robot_label}' has malfunctioned and cannot operate.");
		Templates.Add(TEXT("ROBOT_FAULT"), T);
	}
}

FString FMAEventTemplateRegistry::SubstitutePlaceholders(const FString& MessageTemplate, const TMap<FString, FString>& Params)
{
	FString Result = MessageTemplate;

	for (const auto& Pair : Params)
	{
		const FString Placeholder = FString::Printf(TEXT("{%s}"), *Pair.Key);
		Result = Result.Replace(*Placeholder, *Pair.Value);
	}

	return Result;
}

bool FMAEventTemplateRegistry::HasTemplate(const FString& EventKey)
{
	InitTemplates();
	return Templates.Contains(EventKey);
}

FMARenderedEvent FMAEventTemplateRegistry::RenderEvent(const FString& EventKey, const TMap<FString, FString>& Params)
{
	InitTemplates();

	FMARenderedEvent Rendered;

	const FMAEventTemplate* Tmpl = Templates.Find(EventKey);
	if (!Tmpl)
	{
		// Unknown event key - return a default event
		Rendered.Category = TEXT("unknown");
		Rendered.Type = TEXT("unknown");
		Rendered.Severity = EMAEventSeverity::Abort;
		Rendered.Message = FString::Printf(TEXT("Unknown event key: %s"), *EventKey);
		Rendered.Payload = Params;
		return Rendered;
	}

	Rendered.Category = Tmpl->Category;
	Rendered.Type = Tmpl->Type;
	Rendered.Severity = Tmpl->Severity;
	Rendered.Message = SubstitutePlaceholders(Tmpl->MessageTemplate, Params);
	Rendered.Payload = Params;

	return Rendered;
}
