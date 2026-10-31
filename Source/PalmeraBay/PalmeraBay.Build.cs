using UnrealBuildTool;

public class PalmeraBay : ModuleRules
{
	public PalmeraBay(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"PhysicsCore",
			"Niagara",
			"Json"
		});
	}
}
