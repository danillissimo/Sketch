// Copyright Epic Games, Inc. All Rights Reserved.

#include "SketchCommands.h"

#define LOCTEXT_NAMESPACE "FSketchModule"

void FSketchCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "Sketch", "Bring up Sketch window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
