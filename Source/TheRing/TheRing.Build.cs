using UnrealBuildTool;

public class TheRing : ModuleRules
{
	public TheRing(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets Private/Tests include headers that sit in the module root.
		PrivateIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AudioMixer",
			"AudioExtensions",
			"SignalProcessing"
		});
	}
}
