#include "Core/Onlooker.h"

#include <Interfaces/IPluginManager.h>

#if WITH_EDITOR
	#include <ISettingsEditorModule.h>
#endif

#define LOCTEXT_NAMESPACE "FOnlookerModule"


void FOnlookerModule::StartupModule()
{
	PluginDirectory = IPluginManager::Get().FindPlugin(TEXT("Onlooker"))->GetBaseDir();
	GlobalSettingsFile = PluginDirectory + "/Settings.ini";

	OnlookerSettings = GetMutableDefault<UOnlookerSettings>();

	#if WITH_EDITOR
		OnlookerSettings->OnSettingChanged().AddRaw(this, &FOnlookerModule::ReloadConfiguration);
	#endif
}

void FOnlookerModule::ReloadConfiguration(UObject* Object, struct FPropertyChangedEvent& Property)
{
	const FName PropertyName = Property.GetPropertyName();

	OnlookerSettings->SaveConfig();
}

void FOnlookerModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOnlookerModule, Onlooker)
