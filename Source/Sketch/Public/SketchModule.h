#pragma once
#include "Modules/ModuleManager.h"

class FSketchModule : public IModuleInterface
{
public:
	static FSketchModule& Get() { return FModuleManager::Get().GetModuleChecked<FSketchModule>("Sketch"); }

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();

	TSharedPtr<FUICommandList> Commands;
	TSharedPtr<FWorkspaceItem> SketchMenu;
};
