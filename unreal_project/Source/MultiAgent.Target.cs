using UnrealBuildTool;
using System.Collections.Generic;

public class MultiAgentTarget : TargetRules
{
    public MultiAgentTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        BuildEnvironment = TargetBuildEnvironment.Unique;
        ExtraModuleNames.Add("MultiAgent");
    }
}
