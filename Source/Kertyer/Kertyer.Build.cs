// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Kertyer : ModuleRules
{
	public Kertyer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore","EnhancedInput", "AnimGraphRuntime" , "UMG","GameplayAbilities", "GameplayTags", "GameplayTasks", "Slate", "SlateCore"});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

	}
}
