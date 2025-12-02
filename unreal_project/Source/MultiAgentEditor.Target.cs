using UnrealBuildTool;
using System.Collections.Generic;

public class MultiAgentEditorTarget : TargetRules
{
    public MultiAgentEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V4;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
        ExtraModuleNames.Add("MultiAgent");
    }
}
