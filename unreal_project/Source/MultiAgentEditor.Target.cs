using UnrealBuildTool;
using System.Collections.Generic;

public class MultiAgentEditorTarget : TargetRules
{
    public MultiAgentEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        BuildEnvironment = TargetBuildEnvironment.Unique;
        ExtraModuleNames.Add("MultiAgent");
    }
}
