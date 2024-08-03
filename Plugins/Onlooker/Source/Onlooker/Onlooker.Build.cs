using System.IO;
using UnrealBuildTool;

public class Onlooker : ModuleRules
{
	public Onlooker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		string enginePath = Path.GetFullPath(Target.RelativeEnginePath);

		bLegacyPublicIncludePaths = false;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);

		if (Target.bBuildEditor) {
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"SettingsEditor"
				}
			);
		}

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Projects",
				"DeveloperSettings"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
				{ }
		);
	}
}