#pragma once
#include "Styling/SlateStyle.h"

/** @note Currently unused, but will probably be in the future, so let it hang around */
class FSketchStyle
{
public:
	static const ISlateStyle& Get();

	static void Initialize();
	static void Shutdown();
	static void ReloadTextures();
	static FName GetStyleSetName();

private:
	static TSharedRef<FSlateStyleSet> Create();

private:
	static TSharedPtr<FSlateStyleSet> StyleInstance;
};
