#pragma once

class SSketchWidgetEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchWidgetEditor) {}
		SLATE_ATTRIBUTE(FMargin, ContentPadding)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
