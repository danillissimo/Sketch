#pragma once
#include "HeaderTool/SketchHeaderTool.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SSearchBox.h"

class STextBlock;
class SScrollBox;
class SSketchLog;
class SSearchBox;
class SMultiLineEditableTextBox;
class SSuggestionTextBox;

class SSketchHeaderTool : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchHeaderTool) {}
		SLATE_ARGUMENT(FMargin, ContentPadding)
	SLATE_END_ARGS()

	SKETCH_API void Construct(const FArguments& InArgs);
	SKETCH_API virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	FSlateEditableTextLayout* GetTextLayout();
	void OnSearch(SSearchBox::SearchDirection Direction);

	TSharedRef<SWidget> ListCommonPaths();
	void SelectCommonPath(const FStringView Path);
	void SelectProjectPath(const FStringView Path);

	FReply GenerateCode();
	EVisibility GetButtonTextVisibility() const { return bWorking ? EVisibility::Collapsed : EVisibility::Visible; }
	EVisibility GetButtonThrobberVisibility() const { return bWorking ? EVisibility::Visible : EVisibility::Collapsed; }

	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SScrollBox> ScrollBox;
	TSharedPtr<STextBlock> Lines;
	TSharedPtr<SMultiLineEditableTextBox> GeneratedCode;
	TSharedPtr<SSketchLog> LogViewer;
	TSharedPtr<SEditableTextBox> PathEdit;
	TSharedPtr<SEditableTextBox> InclusionRootEdit;
	bool bWorking = false;

	sketch::HeaderTool::FLog Log;
};
