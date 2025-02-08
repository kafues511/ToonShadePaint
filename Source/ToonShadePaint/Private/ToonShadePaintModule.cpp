// Copyright © 2024-2025 kafues511 All Rights Reserved.

#include "ToonShadePaintModule.h"
#include "ShaderCore.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"

#define LOCTEXT_NAMESPACE "FToonShadePaintModule"

void FToonShadePaintModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ToonShadePaint"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/ToonShadePaint"), PluginShaderDir);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FToonShadePaintModule::RegisterMenus));
}

void FToonShadePaintModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
}

void FToonShadePaintModule::RegisterMenus()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = Menu->AddSection("Coori", LOCTEXT("Coori", "Coori"));
	Section.AddEntry(FToolMenuEntry::InitMenuEntry(
		"ToonShadePaint",
		LOCTEXT("ToonShadePaintTitle", "Toon Shade Paint"),
		LOCTEXT("ToonShadePaintTooltip", "Toon shade paint tool."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.MeshPaintMode"),
		FUIAction(FExecuteAction::CreateRaw(this, &FToonShadePaintModule::OnToonShadePaint))
	));
}

void FToonShadePaintModule::OnToonShadePaint()
{
	UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	UEditorUtilityWidgetBlueprint* EditorUtilityWidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(NULL, TEXT("/ToonShadePaint/Widgets/EUW_ToonShadePaint"), NULL, LOAD_None, NULL);
	EditorUtilitySubsystem->SpawnAndRegisterTab(EditorUtilityWidgetBlueprint);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FToonShadePaintModule, ToonShadePaint)
