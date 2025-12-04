using UnrealBuildTool;
using System.Collections.Generic;

public class MultiAgentTarget : TargetRules
{
    public MultiAgentTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        ExtraModuleNames.Add("MultiAgent");
    }
}
