#pragma once
#include "Modules/ModuleManager.h"

class FUICommandList;
class FWorkspaceItem;

class FSketchModule : public IModuleInterface
{
public:
	static FSketchModule& Get() { return FModuleManager::Get().GetModuleChecked<FSketchModule>("Sketch"); }

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FUICommandList> Commands;
	TSharedPtr<FWorkspaceItem> SketchMenu;
};
