#pragma once
#include "Widgets/SCompoundWidget.h"

class SSketchController : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchController) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Update();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void OnAttributesChanged();
};
