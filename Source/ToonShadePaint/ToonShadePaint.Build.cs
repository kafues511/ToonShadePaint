// Copyright © 2024-2025 kafues511 All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ToonShadePaint : ModuleRules
{
	public ToonShadePaint(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(GetModuleDirectory("Renderer"), "Private"),
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Projects",
				"RenderCore",
				"Renderer",
				"RHI",
				"UnrealEd",
				"AssetTools",
				"AssetRegistry",
				"EditorSubsystem",
				"Blutility",
				"UMGEditor",
				"EditorWidgets",
				"ToolMenus",
			}
		);
	}
}
