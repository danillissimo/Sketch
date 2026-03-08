#pragma once
#include "Framework/Commands/Commands.h"

class FSketchCommands : public TCommands<FSketchCommands>
{
public:
	FSketchCommands();
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OpenWidgetEditor;
	TSharedPtr<FUICommandInfo> OpenSandbox;
	TSharedPtr<FUICommandInfo> OpenHeaderTool;
};
