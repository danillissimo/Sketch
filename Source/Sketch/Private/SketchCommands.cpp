#include "SketchCommands.h"

#include "SketchStyle.h"

#define LOCTEXT_NAMESPACE "FSketchModule"

FSketchCommands::FSketchCommands()
	: TCommands<FSketchCommands>(
		TEXT("Sketch"),
		NSLOCTEXT("Contexts", "Sketch", "Sketch"),
		NAME_None,
		FSketchStyle::GetStyleSetName()
	) {
}

void FSketchCommands::RegisterCommands()
{
	UI_COMMAND(OpenWidgetEditor, "Widget editor", "Open the Sketch widget editor tab", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenSandbox, "Sandbox", "Open the Sketch sandbox tab", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenHeaderTool, "Header tool", "Open the Sketch header tool tab", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
