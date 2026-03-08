#pragma once
#include "Widgets/SCompoundWidget.h"

class SMultiLineEditableTextBox;

class SKETCH_API SSketchWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<SWidgetSwitcher> Switcher;
	FReply Switch(int Index);
};
