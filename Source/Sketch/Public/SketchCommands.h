// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "SketchStyle.h"

class FSketchCommands : public TCommands<FSketchCommands>
{
public:

	FSketchCommands()
		: TCommands<FSketchCommands>(TEXT("Sketch"), NSLOCTEXT("Contexts", "Sketch", "Sketch Plugin"), NAME_None, FSketchStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};