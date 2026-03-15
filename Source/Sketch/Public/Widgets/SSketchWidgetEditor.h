#pragma once
#include "SSketchWidget.h"

class SSketchOutliner;

class SSketchWidgetEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchWidgetEditor) {}
		SLATE_ATTRIBUTE(FMargin, ContentPadding)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void OnTargetChanged(SSketchWidget* SketchWidget);
	static FReply OnPreviewOverlayDoubleClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent);

	TSharedPtr<SSketchOutliner> Outliner;
	TSharedPtr<SSketchWidget> WidgetPreview;
	TSharedPtr<SOverlay> WidgetPreviewOverlay;
};
