#pragma once
#include "Widgets/SCompoundWidget.h"

class SSketchHeaderRow;

class SSketchSandbox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchSandbox) {}
		SLATE_ATTRIBUTE(FMargin, ContentPadding)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void ScheduleUpdate();
	void UpdateAttributes();

	TSharedPtr<SSketchHeaderRow> HeaderRow;
	TSharedPtr<SScrollBox> ScrollBox;
};
